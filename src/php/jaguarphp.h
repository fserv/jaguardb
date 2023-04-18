
#ifndef PHP_JAGUAR_H
#define PHP_JAGUAR_H
 
#define PHP_JAGUAR_EXTNAME  "jaguarphp"
#define PHP_JAGUAR_EXTVER   "1.0"
 
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif 
 
extern "C" {
#include "php.h"
}
 
extern zend_module_entry JAGUARPHP_module_entry;
#define phpext_JAGUAR_ptr &JAGUARPHP_module_entry;
 
#endif /* PHP_JAGUAR_H */

