set(srcs "spinet_main.c"
         "esp_eth_mac_spinet.c"
		 "esp_eth_phy_spinet.c"
		 "cmd_ethernet.c"
		 "ncovif.c"
		 "ipc.c")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS ".")
