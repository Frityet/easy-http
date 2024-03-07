---@diagnostic disable: lowercase-global
rockspec_format = "3.0"
package = "easy-http"
version = "dev-1"
source = {
   url = "*** please add URL for source tarball, zip or repository here ***"
}
description = {
   summary = "Easy and portable HTTP(s) requests",
   homepage = "*** please enter a project homepage ***",
   license = "MIT"
}
dependencies = {
   "lua >= 5.1, < 5.5"
}
external_dependencies = {
   CURL = {
      library = "curl"
   }
}
test = {
   type = "busted",
}
build = {
   type = "builtin",
   modules = {
      easyhttp = {
         incdirs = {
            "$(CURL_INCDIR)"
         },
         libdirs = {
            "$(CURL_LIBDIR)"
         },
         libraries = {
            "curl"
         },
         sources = {"easyhttp.c", "async.c", "compat-5.3.c"}
      }
   }
}
