//
// Created by Mahamadou DOUMBIA [OML DSI] on 10/02/2022.
//

#ifndef MOSC_RANDOM_H
#define MOSC_RANDOM_H

#include "../common/common.h"
#include "../api/msc.h"

#if MSC_OPT_KUNFE

const char* MSCKunfeSource();
MSCExternClassMethods MSCKunfeBindExternClass(MVM* djuru,
                                                   const char* module,
                                                   const char* className);
MSCExternMethodFn MSCKunfeBindExternMethod(MVM* vm,
                                                const char* className,
                                                bool isStatic,
                                                const char* signature);

#endif
#endif //MOSC_RANDOM_H
