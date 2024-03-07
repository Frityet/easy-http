rockspec_format = "3.0"
package = "easy-http"
version = "0.1.0-1"
source = {
   url = "git+https://github.com/Frityet/easy-http.git"
}
description = {
   summary = "Easy and portable HTTP(s) requests",
   homepage = "https://github.com/Frityet/easy-http",
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
build = {
   type = "builtin",
   modules = {
      easyhttp = {
         incdirs = {
            "$(CURL_INCDIR)",
            "src"
         },
         libdirs = {
            "$(CURL_LIBDIR)"
         },
         libraries = {
            "curl"
         },
         sources = {
            "src/easyhttp.c",
            "src/async.c",
            "src/extern/compat-5.3.c",
            "src/extern/tinycthread.c"
         }
      }
   }
}
test = {
   type = "busted"
}
