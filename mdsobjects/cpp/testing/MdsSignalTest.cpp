#include <mdsobjects.h>

#include "testing.h"
#include "testutils/unique_ptr.h"
#include "mdsplus/AutoPointer.hpp"

using namespace MDSplus;
using namespace testing;


////////////////////////////////////////////////////////////////////////////////
//  SIGNAL COMPOUND  ///////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

///
/// \brief Test of the Signal class object, description of DTYPE_SIGNAL 
///

//class Signal: public Compound {
//public:
//    Signal(int dtype, int length, char *ptr, int nDescs, char **descs, Data *units = 0, Data *error = 0, Data *help = 0, Data *validation = 0):
//        Compound(dtype, length, ptr, nDescs, descs);
//    Signal(Data *data, Data *raw, Data *dimension, Data *units = 0, Data *error = 0, Data *help = 0, Data *validation = 0);
//    Signal(Data *data, Data *raw, Data *dimension1, Data *dimension2, Data *units = 0, Data *error = 0, Data *help = 0, Data *validation = 0);
//    Signal(Data *data, Data *raw, int nDims, Data **dimensions, Data *units = 0, Data *error = 0, Data *help = 0, Data *validation = 0);

//    Data *getData() { return getDescAt(0); }
//    Data *getRaw() { return getDescAt(1); }
//    Data *getDimension() { return getDescAt(2); }
//    Data *getDimensionAt(int idx) { return getDescAt(idx + 2); }
//    int getNumDimensions() { return descs.size() - 2; }
//    void setData(Data *data) {assignDescAt(data, 0);}
//    void setRaw(Data *raw){assignDescAt(raw, 1);}
//    void setDimension(Data *dimension) {assignDescAt(dimension, 2);}
//    void setDimensionAt(Data *dimension, int idx) {assignDescAt(dimension, 2 + idx);}

//};


int main(int argc UNUSED_ARGUMENT, char *argv[] UNUSED_ARGUMENT)
{
    BEGIN_TESTING(Signal);

    Data * raw[200];
    for(int i=0; i<200; ++i) raw[i] = new Float32(i);
        
    { // CTR
        char dummy_op_code = 0;
        unique_ptr<Signal> ctr1 = new Signal(0,sizeof(dummy_op_code),&dummy_op_code,200,(char **)raw);
        unique_ptr<Signal> ctr2 = new Signal(new String("Value"), new Float32(5552368), new Float32(1));
        unique_ptr<Signal> ctr3 = new Signal(new String("Value"), new Float32(5552368), new Float32(1), new Float32(2), 0,0,0,0);
        unique_ptr<Signal> ctr4 = new Signal(new String("Value"), new Float32(5552368), 3, (Data**)raw);        
        TEST1( ctr2->getNumDimensions() == 1 );
        TEST1( ctr3->getNumDimensions() == 2 );
        TEST1( ctr4->getNumDimensions() == 3 );        
    }

    unique_ptr<Signal> sig = new Signal(new String("Value"), new Float32(5552368), new Float32(1), new Float32(2), 0,0,0,0);
    TEST1( AutoString(AutoData<Data>(sig->getData())->getString()).string == std::string("Value") );
    TEST1( unique_ptr<Float32>((Float32*)sig->getRaw())->getFloat() == 5552368.);
    TEST1( unique_ptr<Float32>((Float32*)sig->getDimension())->getFloat() == 1.);
    TEST1( unique_ptr<Float32>((Float32*)sig->getDimensionAt(1))->getFloat() == 2.);
            
    { // Set-Get dimensions
        sig->setDimension(new Float32(5.55));
        sig->setDimensionAt(new Float32(23.68),1);
        TEST1( unique_ptr<Float32>((Float32*)sig->getDimension())->getFloat() == (float)5.55);
        TEST1( unique_ptr<Float32>((Float32*)sig->getDimensionAt(1))->getFloat() == (float)23.68);
    }
    
    
    END_TESTING;
}
