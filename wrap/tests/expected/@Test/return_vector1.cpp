// automatically generated by wrap on 2011-Dec-08
#include <wrap/matlab.h>
#include <folder/path/to/Test.h>
void mexFunction(int nargout, mxArray *out[], int nargin, const mxArray *in[])
{
  checkArguments("return_vector1",nargout,nargin-1,1);
  shared_ptr<Test> self = unwrap_shared_ptr< Test >(in[0],"Test");
  Vector value = unwrap< Vector >(in[1]);
  Vector result = self->return_vector1(value);
  out[0] = wrap< Vector >(result);
}
