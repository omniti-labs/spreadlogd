#include "perl.h"
#include <EXTERN.h>
#include <perl.h>

EXTERN_C void xs_init();

static PerlInterpreter *my_perl = NULL;

static SV *my_eval_sv(SV *sv, I32 coe) {
    dSP;
    SV *retval;
    STRLEN n_a;

    PUSHMARK(SP);
    eval_sv(sv, G_SCALAR);

    SPAGAIN;
    retval = POPs;
    PUTBACK;

    if(coe && SvTRUE(ERRSV)) {
        croak(SvPVx(ERRSV, n_a));
    }
    return retval;
}

void perl_startup() {
    char *embedding[] = { "", "-e", "0" };

    my_perl = perl_alloc();
    perl_construct( my_perl );

    perl_parse(my_perl, xs_init, 3, embedding, NULL);
}

void perl_shutdown() {
    if(my_perl) {
        perl_destruct(my_perl);
        perl_free(my_perl);
        my_perl = NULL;
    }
}

I32 perl_inc(char *path) {
    SV *retval;
    SV *command = NEWSV(1024+20, 0);

    sv_setpvf(command, "use lib '%s';", path);
    retval = my_eval_sv(command, TRUE);
    SvREFCNT_dec(command);

    return SvIV(retval);
}

I32 perl_use(char *module) {
    SV *retval;
    SV *command = NEWSV(1024+20, 0);

    sv_setpvf(command, "use %s;", module);
    retval = my_eval_sv(command, TRUE);
    SvREFCNT_dec(command);

    return SvIV(retval);
}

I32 perl_log(char *func, char *sender, char *group, char *message) {
    int retval;

    dSP;                                       /* initialize stack pointer      */
    ENTER;                                     /* everything created after here */
    SAVETMPS;                                  /* ...is a temporary variable.   */
    PUSHMARK(SP);                              /* remember the stack pointer    */
    XPUSHs(sv_2mortal(newSVpv(sender,0)));      /* push the base onto the stack  */
    XPUSHs(sv_2mortal(newSVpv(group,0)));      /* push the base onto the stack  */
    XPUSHs(sv_2mortal(newSVpv(message, 0)));  /* push the exponent onto stack  */
    PUTBACK;                                   /* make local stack pointer global */
    call_pv(func, G_SCALAR);                   /* call the function             */
    SPAGAIN;                                   /* refresh stack pointer         */
                                               /* pop the return value from stack */
    retval = POPi;
    PUTBACK;
    FREETMPS;                                  /* free that return value        */
    LEAVE;                                     /* ...and the XPUSHed "mortal" args.*/
    return retval;
}

I32 perl_hup(char *func) {
    int retval;

    dSP;                                  /* initialize stack pointer      */
    ENTER;                                /* everything created after here */
    SAVETMPS;                             /* ...is a temporary variable.   */
    PUSHMARK(SP);                         /* remember the stack pointer    */
    PUTBACK;                              /* make local stack pointer global */
    call_pv(func, G_SCALAR);              /* call the function             */
    SPAGAIN;                              /* refresh stack pointer         */
                                          /* pop the return value from stack */
    retval = POPi;
    PUTBACK;
    FREETMPS;                             /* free that return value        */
    LEAVE;                                /* ...and the XPUSHed "mortal" args.*/
    return retval;
}
