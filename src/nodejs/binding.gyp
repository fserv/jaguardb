{
  "targets": [
    {
      "target_name": "jaguarnode",
	  "include_dirs": ["$(HOME)/src"],
      "sources": [
        "Jaguar.cc",
        "JagAPI.cc",
        "JagAPI.h",
        "JaguarAPI.h",
      ],
	  "link_settings": {
  	    "libraries":["-L$(JAGUAR_HOME)/lib -lJaguarClient"]
	  },
    }
  ]
}
