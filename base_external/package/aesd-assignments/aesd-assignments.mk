##############################################################
#
# AESD-ASSIGNMENTS
# modification: 27DEC2025, by Arnaud SIMO
# Works for builds root and qemu: 30DEC2025
##############################################################

AESD_ASSIGNMENTS_VERSION = 8dbe362 
AESD_ASSIGNMENTS_SITE =  git@github.com:simo-2021/Embedded-Temperature-Sensor-Calibrator.git
AESD_ASSIGNMENTS_SITE_METHOD = git
AESD_ASSIGNMENTS_GIT_SUBMODULES = YES

define AESD_ASSIGNMENTS_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)/finder-app  all
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)/server  all
endef

define AESD_ASSIGNMENTS_INSTALL_TARGET_CMDS
	# Configuration
	$(INSTALL) -d -m 0755 $(TARGET_DIR)/etc/finder-app/conf
	$(INSTALL) -m 0644 $(@D)/finder-app/conf/* $(TARGET_DIR)/etc/finder-app/conf/

	# Binaire writer (and aesdsocket)
	$(INSTALL) -m 0755 $(@D)/finder-app/writer $(TARGET_DIR)/usr/bin/
	$(INSTALL) -m 0755 $(@D)/finder-app/writer $(TARGET_DIR)/etc/finder-app/

    	$(INSTALL) -m 0755 $(@D)/server/aesdsocket  $(TARGET_DIR)/usr/bin/
	
   	# Installer le script finder-test.sh et finder.sh (and aesdsocket-start-stop 
    	$(INSTALL) -m 0755 $(@D)/finder-app/finder-test.sh $(TARGET_DIR)/usr/bin/
    	$(INSTALL) -m 0755 $(@D)/finder-app/finder.sh $(TARGET_DIR)/usr/bin/
    	
    	$(INSTALL) -m 0755 $(@D)/server/aesdsocket-start-stop.sh  $(TARGET_DIR)/etc/init.d/S99aesdsocket
    	    	
    	# Installer capteur température
	$(INSTALL) -m 0755 $(@D)/server/temp_sensor $(TARGET_DIR)/usr/bin/

	# Installer service TCP
	$(INSTALL) -m 0755 $(@D)/server/temp_server $(TARGET_DIR)/usr/bin/

	# Installer script de démarrage (S98 : avant aesdsocket)
	$(INSTALL) -m 0755 $(@D)/script/temp_service-start-stop.sh $(TARGET_DIR)/etc/init.d/S98temp_service
endef

$(eval $(generic-package))
