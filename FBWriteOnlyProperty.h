/**********************************************************\
Original Author: Andrew Ofisher (zaltar)

License:    GNU General Public License, version 3.0
            http://www.gnu.org/licenses/gpl-3.0.txt

Copyright 2012 Andrew Ofisher
\**********************************************************/

#include "JSExceptions.h"
#include "PropertyConverter.h"

namespace FB { 
    namespace detail { 
		namespace properties {
			inline int dummyGetter()
			{
				throw FB::script_error("Access denied.");
			}
		} 
	} 

	/*! A convient template to create a write only property
	 *  on the JavaScript side. Calling the getter will simply
	 *  throw a JavaScript exception.
	 */
    template<class C, typename F1>
    inline PropertyFunctors 
    make_write_only_property(C* instance, F1 f1)
    {
        return PropertyFunctors(
			boost::bind(FB::detail::properties::dummyGetter),
            FB::detail::properties::setter<C, F1>::result::f(instance, f1));
    }
}
