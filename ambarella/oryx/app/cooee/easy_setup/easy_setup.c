#include "easy_setup.h"

#include "../proto/cooee.h"
#include "../proto/neeze.h"
#include "../proto/akiss.h"
#include "../proto/changhong.h"

extern uint16 g_protocol_mask;

void easy_setup_enable_cooee() {
    g_protocol_mask |= (1<<EASY_SETUP_PROTO_COOEE);
}

void easy_setup_enable_neeze() {
    g_protocol_mask |= (1<<EASY_SETUP_PROTO_NEEZE);
}

void easy_setup_enable_akiss() {
    g_protocol_mask |= (1<<EASY_SETUP_PROTO_AKISS);
}

void easy_setup_enable_changhong() {
    g_protocol_mask |= (1<<EASY_SETUP_PROTO_CHANGHONG);
}

void easy_setup_enable_protocols(uint16 proto_mask) {
    g_protocol_mask |= proto_mask;
}

void easy_setup_get_param(uint16 proto_mask, tlv_t** pptr) {
    tlv_t* t = *pptr;
    int i=0;
    for (i=0; i<EASY_SETUP_PROTO_MAX; i++) {
        if (proto_mask & (1<<i)) {
            t->type = i;

            if (i==EASY_SETUP_PROTO_COOEE) {
                t->length = sizeof(cooee_param_t);
                cooee_get_param(t->value);
            } else if (i==EASY_SETUP_PROTO_NEEZE) {
                t->length = sizeof(neeze_param_t);
                neeze_get_param(t->value);
            } else if (i==EASY_SETUP_PROTO_AKISS) {
                t->length = sizeof(akiss_param_t);
                akiss_get_param(t->value);
            } else {
                t->length = 0;
            }

            t = (tlv_t*) (t->value + t->length);
        }
    }

    *pptr = t;
}

void easy_setup_set_result(uint8 protocol, void* p) {
    if (protocol == EASY_SETUP_PROTO_COOEE) {
        cooee_set_result(p);
    } else if (protocol == EASY_SETUP_PROTO_NEEZE) {
        neeze_set_result(p);
    } else if (protocol == EASY_SETUP_PROTO_AKISS) {
        akiss_set_result(p);
    } else if (protocol == EASY_SETUP_PROTO_CHANGHONG) {
        changhong_set_result(p);
    } else {
        ;// nothing done
    }
}
