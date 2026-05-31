#include "plugin.hpp"

Plugin* pluginInstance;

extern Model* modelGinaArp;
extern Model* modelGinasExpander;
extern Model* modelProtoseqBlank;
extern Model* modelArc;

void init(Plugin* p) {
	pluginInstance = p;
	p->addModel(modelGinaArp);
	p->addModel(modelGinasExpander);
	p->addModel(modelProtoseqBlank);
	p->addModel(modelArc);
}
