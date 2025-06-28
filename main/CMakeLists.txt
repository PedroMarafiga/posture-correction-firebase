# Embed CA, certificate & key directly into binary
idf_component_register(SRCS "wpa2_enterprise_main.cpp"
                    INCLUDE_DIRS "." "../components"
                    EMBED_TXTFILES ca.pem wpa2_ca.pem wpa2_client.crt wpa2_client.key)
