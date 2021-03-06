import java.beans.*;
import javax.swing.*;
import java.awt.*;

public class DeviceParametersBeanInfo extends SimpleBeanInfo
{
    public PropertyDescriptor property(String name, String description)
    throws IntrospectionException
    {
        PropertyDescriptor p = new PropertyDescriptor(name, DeviceParameters.class);
        p.setShortDescription(description);
        return p;
    }
    public Image getIcon(int kind)
    {
        return loadImage("structure.gif");
    }

    public PropertyDescriptor [] getPropertyDescriptors()
    {
        try {
            PropertyDescriptor[] props = {
               property("offsetNid", "Offset nid"),
               property("baseName", "Base name")
             };
            return props;
        }catch(IntrospectionException e)
        {
            System.out.println("DeviceParameters: property exception " + e);
            return super.getPropertyDescriptors();
        }
    }

}
