// automatically generated by wrap on 2011-Dec-08
#include <wrap/matlab.h>
#include <folder/path/to/Test.h>
void mexFunction(int nargout, mxArray *out[], int nargin, const mxArray *in[])
{
  checkArguments("return_double",nargout,nargin-1,1);
  shared_ptr<Test> self = unwrap_shared_ptr< Test >(in[0],"Test");
  double value = unwrap< double >(in[1]);
  double result = self->return_double(value);
  out[0] = wrap< double >(result);
}
