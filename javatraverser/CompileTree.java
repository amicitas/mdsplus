import javax.xml.parsers.*;
import org.xml.sax.*;
import org.xml.sax.helpers.*;
import org.w3c.dom.*;
import java.util.*;


public class CompileTree extends Thread
{
    Database tree;
    String experiment;
    int shot;
    
    //originalNames and renamedNames keep info about nodes to be renamed
    Vector renamedDevices = new Vector();
    Vector renamedFieldNids = new Vector();
    Vector newNames = new Vector();
    
    
    public static void main(String args[])
    {
        String experiment;
        int shot = -1;
        if(args.length < 1)
        {
            
            System.out.println("Usage: java CompileTree <experiment> [<shot>]");
            System.exit(0);
        }
        experiment = args[0];

        if(args.length > 1)
        {
            try {
                shot = Integer.parseInt(args[1]);
            }catch(Exception exc)
            {
                System.out.println("Error Parsing shot number");
                System.exit(0);
            }
        }
        (new CompileTree(experiment, shot)).start(); 
    }
      
        
    CompileTree(String experiment, int shot)
    {
        this.experiment = experiment;
        this.shot = shot;
    }
        
    public void run()
    {
        DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
        Document document = null;
        try {
            DocumentBuilder builder = factory.newDocumentBuilder();
            document = builder.parse(experiment + ".xml");
        } catch (SAXParseException spe) {
           // Error generated by the parser
           System.out.println("\n** Parsing error"
              + ", line " + spe.getLineNumber()
              + ", uri " + spe.getSystemId());
           System.out.println("   " + spe.getMessage() );

           // Use the contained exception, if any
           Exception  x = spe;
           if (spe.getException() != null)
               x = spe.getException();
           x.printStackTrace();
 
        } catch (SAXException sxe) {
           // Error generated during parsing)
           Exception  x = sxe;
           if (sxe.getException() != null)
               x = sxe.getException();
           x.printStackTrace();

        } catch (ParserConfigurationException pce) {
            // Parser with specified options can't be built
            pce.printStackTrace();

        } catch (Exception ioe) {
           // I/O error
           ioe.printStackTrace();
        }
        
        
        tree = new Database(experiment, shot);
        tree.setEditable(true);
        try {
            tree.openNew();
        }catch(Exception exc)
        {
            System.out.println("Error opening tree " + experiment +" : " + exc);
            System.exit(0);
        }
        
        Element rootNode = document.getDocumentElement();
        NodeList nodes = rootNode.getChildNodes();
        for(int i = 0; i < nodes.getLength(); i++)
        {
            org.w3c.dom.Node currNode = nodes.item(i);
            if(currNode.getNodeType() == org.w3c.dom.Node.ELEMENT_NODE) //Only element nodes at this
                recCompile((Element)currNode);
        }
        
        
        //handle renamed nodes
        for(int i = 0; i < newNames.size(); i++)
        {
            String newName = (String)newNames.elementAt(i);
            String deviceName = (String)renamedDevices.elementAt(i);
            String offsetStr = (String)renamedFieldNids.elementAt(i);
            try {
                int deviceOffset = Integer.parseInt(offsetStr);
                NidData deviceNid = tree.resolve(new PathData(deviceName), 0);
                NidData renamedNid = new NidData(deviceNid.getInt()+deviceOffset);
                tree.renameNode(renamedNid, newName, 0);
            }catch(Exception exc)
            {
                System.out.println("Error renaming node of " + deviceName + " to " + newName + " : " + exc);
            }
        }
        try {
            tree.write(0);
            tree.close(0);
        }catch(Exception exc)
        {
            System.out.println("Error closeing tree: " + exc);
        }
    }
    
    void recCompile(Element node)
    {
        String type = node.getNodeName();
        String name = node.getAttribute("NAME");
        String state = node.getAttribute("STATE");
        String usageStr = node.getAttribute("USAGE");
        NidData nid = null;
        boolean success;
        try {
            NidData parentNid = tree.getDefault(0);
            success = false;
            if(type.equals("data")) 
            {
                Element parentNode = (Element)node.getParentNode();
                boolean isDeviceField = node.getNodeName().equals("field");
                Text dataNode = (Text)node.getFirstChild();
                if(dataNode != null)
                {
                    String dataStr = dataNode.getData();
                    
                    Data data = null;
                    try {
                        data = Data.fromExpr(dataStr);
                    }catch(Exception exc)
                    {
                        System.out.println("Error parsing expression " + dataStr + " : " + exc);
                    }
                    try {
                        nid = tree.getDefault(0);
                        if(isDeviceField)
                        {
                            Data oldData;
                            try {
                                oldData = tree.getData(nid, 0);
                            }catch(Exception exc) {oldData = null; }
                            if(oldData == null || !dataStr.equals(oldData.toString()))
                                tree.putData(nid, data, 0);
                        }
                        else
                            tree.putData(nid, data, 0);
                    }catch(Exception exc)
                    {
                        System.out.println("Error writing data: " + exc);
                    }
                }
                return;
            }

            //First handle renamed nodes: they do not need to be created, but to be renamed
            String originalDevice = node.getAttribute("DEVICE");
            String deviceOffsetStr = node.getAttribute("OFFSET_NID");
            if(originalDevice != null && deviceOffsetStr != null && 
                !originalDevice.equals("") && !deviceOffsetStr.equals(""))
            {
                String newName;
                try {
                    newName = (tree.getInfo(parentNid, 0)).getFullPath();
                }catch(Exception exc)
                {
                    System.err.println("Error getting renamed path: " + exc);
                    return;
                }
                if(type.equals("node"))
                    newName += "." + name;
                else
                    newName += ":" + name;
                newNames.addElement(newName);
                renamedDevices.addElement(originalDevice);
                renamedFieldNids.addElement(deviceOffsetStr);
                return; //No descedents for a renamed node
            }

            if(type.equals("node"))
            {
                try  {
                    nid = tree.addNode("."+name, NodeInfo.USAGE_STRUCTURE, 0);
                    if(usageStr != null && usageStr.equals("SUBTREE"))
                        tree.setSubtree(nid, 0);
                    tree.setDefault(nid, 0);
                    success = true;
                }catch(Exception exc)
                {
                    System.out.println("Error adding member " + name + " : "+ exc);
                }
            }
            if(type.equals("member"))
            {
                int usage = NodeInfo.USAGE_NONE;
                if(usageStr.equals("NONE")) usage = NodeInfo.USAGE_NONE;
                if(usageStr.equals("ACTION")) usage = NodeInfo.USAGE_ACTION;
                if(usageStr.equals("NUMERIC")) usage = NodeInfo.USAGE_NUMERIC;
                if(usageStr.equals("SIGNAL")) usage = NodeInfo.USAGE_SIGNAL;
                if(usageStr.equals("TASK")) usage = NodeInfo.USAGE_TASK;
                if(usageStr.equals("TEXT")) usage = NodeInfo.USAGE_TEXT;
                if(usageStr.equals("WINDOW")) usage = NodeInfo.USAGE_WINDOW;
                if(usageStr.equals("AXIS")) usage = NodeInfo.USAGE_AXIS;
                if(usageStr.equals("DISPATCH")) usage = NodeInfo.USAGE_DISPATCH;
                try {
                    nid = tree.addNode(":"+name, usage, 0);
                    tree.setDefault(nid, 0);
                    success = true;
                }catch(Exception exc)
                {
                    System.out.println("Error adding member " + name + " : "+ exc);
                }
            }
            
            if(type.equals("device"))
            {
                String model = node.getAttribute("MODEL");
                NodeInfo info = tree.getInfo(parentNid, 0);
                
                try {
                    Thread.currentThread().sleep(100);
                    
                    nid = tree.addDevice(name.trim(), model, 0);
                    if(nid != null)
                    {
                        tree.setDefault(nid, 0);
                        success = true;
                    }
                }
                catch(Exception exc){}
            }
            
            if(type.equals("field"))
            {
                nid= tree.resolve(new PathData(name), 0);
                tree.setDefault(nid, 0);
                success = true;
            }
            if(success)
            {
                //tags
                String tagsStr = node.getAttribute("TAGS");
                if(tagsStr != null && tagsStr.length() > 0)
                {
                    int i = 0;
                    StringTokenizer st = new StringTokenizer(tagsStr, ", ");
                    String [] tags = new String[st.countTokens()];
                    while(st.hasMoreTokens())
                        tags[i++] = st.nextToken();
                    try
                    {
                        tree.setTags(nid, tags, 0);
                    }catch(Exception exc)
                    {
                        System.out.println("Error adding tags " + tagsStr + " : " + exc);
                    }
                }
                
                //flags
                String flagsStr = node.getAttribute("FLAGS");
                if(flagsStr != null && flagsStr.length() > 0)
                {
                    int flags = 0;
                    StringTokenizer st = new StringTokenizer(flagsStr, ", ");
                    while(st.hasMoreTokens())
                    {
                        String flag = st.nextToken();
                        if(flag.equals("WRITE_ONCE"))
                            flags |= NodeInfo.WRITE_ONCE;
                        if(flag.equals("COMPRESSIBLE"))
                            flags |= NodeInfo.COMPRESSIBLE;
                        if(flag.equals("COMPRESS_ON_PUT"))
                            flags |= NodeInfo.COMPRESS_ON_PUT;
                        if(flag.equals("NO_WRITE_MODEL"))
                            flags |= NodeInfo.NO_WRITE_MODEL;
                        if(flag.equals("NO_WRITE_SHOT"))
                            flags |= NodeInfo.NO_WRITE_SHOT;
                    }
                    try {
                       tree.setFlags(nid, flags);
                    }catch(Exception exc)
                    {
                        System.out.println("Error setting flags to node " + name + " : " + exc);
                    }
                }
                
                //state
                String stateStr = node.getAttribute("STATE");
                if(stateStr != null && stateStr.length() > 0)
                {
                    try {
                     if(stateStr.equals("ON"))
                        tree.setOn(nid, true, 0);
                    if(stateStr.equals("OFF"))
                        tree.setOn(nid, false, 0);
                    }catch(Exception exc)
                    {
                        //System.out.println("Error setting state of node " + name + " : " + exc);
                    }
                }
                
                //Descend
                NodeList nodes = node.getChildNodes();
                for(int i = 0; i < nodes.getLength(); i++)
                {
                    org.w3c.dom.Node currNode = nodes.item(i);
                    if(currNode.getNodeType() == org.w3c.dom.Node.ELEMENT_NODE) //Only element nodes at this
                        recCompile((Element)currNode);
                }
            }
            tree.setDefault(parentNid, 0);
        }catch(Exception exc)
        {
            System.out.println("Internal error in recCompile: " + exc);
        }
    }
    
     
}
            
