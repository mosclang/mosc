//
// Created by Mahamadou DOUMBIA [OML DSI] on 10/02/2022.
//

#ifndef MOSC_FAN_H
#define MOSC_FAN_H


#include "../common/common.h"
#include "../api/msc.h"

// This module defines the Meta class and its associated methods.
#if MSC_OPT_FAN

const char* MSCFanSource();
MSCExternMethodFn MSCFanBindExternMethod(MVM* vm,
                                              const char* className,
                                              bool isStatic,
                                              const char* signature);

#endif

#endif //MOSC_FAN_H
