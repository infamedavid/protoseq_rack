#include "plugin.hpp"

Plugin* pluginInstance;

extern Model* modelGinaArp;

void init(Plugin* p) {
	pluginInstance = p;
	p->addModel(modelGinaArp);
}
