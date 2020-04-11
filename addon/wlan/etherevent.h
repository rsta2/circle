#ifndef _etherevent_h
#define _etherevent_h

typedef enum
{
	ether_event_disassoc,
	ether_event_deauth,
	ether_event_mic_error,
}
ether_event_type_t;

typedef union
{
	struct
	{
		int		group;		// group key instead of pairwise
		unsigned char	addr[6];
	}
	mic_error;
}
ether_event_params_t;

typedef void ether_event_handler_t (ether_event_type_t		 type,
				    const ether_event_params_t	*params,
				    void			*context);

#endif
