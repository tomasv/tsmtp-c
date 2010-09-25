#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <resolv.h>

int main (int argc, char *argv[])
{
    u_char nsbuf[4096];
    char dispbuf[4096];
    ns_msg msg;
    ns_rr rr;
    int i, j, l;

    if (argc < 2) {
        printf ("Usage: %s <domain>[...]\n", argv[0]);
        exit (1);
    }

    for (i = 1; i < argc; i++) {
        l = res_query (argv[i], ns_c_any, ns_t_mx, nsbuf, sizeof (nsbuf));
        if (l < 0) {
            perror (argv[i]);
        } else {
#ifdef USE_PQUERY
/* this will give lots of detailed info on the request and reply */
            res_pquery (&_res, nsbuf, l, stdout);
#else
/* just grab the MX answer info */
            ns_initparse (nsbuf, l, &msg);
            printf ("%s :\n", argv[i]);
            l = ns_msg_count (msg, ns_s_an);
            for (j = 0; j < l; j++) {
                ns_parserr (&msg, ns_s_an, j, &rr);
                ns_sprintrr (&msg, &rr, NULL, NULL, dispbuf, sizeof (dispbuf));
                printf ("%s\n", dispbuf);
            }
#endif
        }
    }

    exit (0);
}

