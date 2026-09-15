#if defined __extended_hud_informer_src_default_objects_registrar_included
    #endinput
#endif
#define __extended_hud_informer_src_default_objects_registrar_included

#include <amxmodx>
#include <ParamsController>
#include <ExtendedHudInformer>

#include "ExtendedHudInformer/DefaultObjects/ParamType/InformerObject"

DefaultObjects_Register() {
    DefaultObjects_ParamType_Informer_Register();
}
