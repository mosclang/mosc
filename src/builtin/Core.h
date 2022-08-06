//
// Created by Mahamadou DOUMBIA [OML DSI] on 13/01/2022.
//

#ifndef CPMSC_CORE_H
#define CPMSC_CORE_H

#include "../memory/Value.h"


typedef struct  {

    Class * boolClass;
    Class * classClass;
    Class * djuruClass;
    Class * fnClass;
    Class * listClass;
    Class * mapClass;
    Class * nullClass;
    Class * numClass;
    Class * objectClass;
    Class * rangeClass;
    Class * stringClass;


} Core;

void MSCInitCore(Core* core, MVM* vm);
Class *defineClass(MVM *vm, Module *module, const char *name);


#endif //CPMSC_CORE_H
