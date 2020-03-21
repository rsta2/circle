#ifndef _etherevent_h
#define _etherevent_h

typedef enum
{
	ether_event_disassociate,
}
ether_event_type_t;

typedef void ether_event_handler_t (ether_event_type_t type, const void *params, void *context);

#endif
