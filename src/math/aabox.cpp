
#include "math/aabox.h"

AABox::AABox() {}

AABox::AABox(InputStream *stream) {
	low = Vector3f(stream);
	high = Vector3f(stream);
}
