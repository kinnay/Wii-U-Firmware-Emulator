
#include "common/color4f.h"

Color4f::Color4f() {
	r = g = b = a = 0;
}

Color4f::Color4f(float r, float g, float b, float a) {
	this->r = r;
	this->g = g;
	this->b = b;
	this->a = a;
}

Color4f::Color4f(InputStream *stream) {
	r = stream->f32();
	g = stream->f32();
	b = stream->f32();
	a = stream->f32();
}
