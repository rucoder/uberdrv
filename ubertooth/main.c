#include <linux/init.h>
#include <linux/module.h>
#include <linux/usb.h>

MODULE_LICENSE("Dual BSD/GPL");

enum devicetype {
    DEVICE_UBERTOOTH_UNKNOWN  = -1,
    DEVICE_UBERTOOTH_ZERO  = 0,
    DEVICE_UBERTOOTH_ONE  = 1,      
    DEVICE_UBERTOOTH_BOOTLOADER  = 2,
};

static struct usb_device_id usb_ids[] = {
    { USB_DEVICE(0x1d50, 0x6000), .driver_info = DEVICE_UBERTOOTH_UNKNOWN },
    { USB_DEVICE(0x1d50, 0x6001), .driver_info = DEVICE_UBERTOOTH_UNKNOWN },
    { USB_DEVICE(0x1d50, 0x6002), .driver_info = DEVICE_UBERTOOTH_ONE },
    { USB_DEVICE(0x1d50, 0x6003), .driver_info = DEVICE_UBERTOOTH_BOOTLOADER },
    { USB_DEVICE(0xffff, 0x0004), .driver_info = DEVICE_UBERTOOTH_UNKNOWN },
    {}
};

MODULE_AUTHOR("Michael Malyshev");
MODULE_VERSION("1.0");  
MODULE_DEVICE_TABLE(usb, usb_ids);


static inline u16 get_bcdDevice(const struct usb_device *udev)
{
    return le16_to_cpu(udev->descriptor.bcdDevice);
}


static const char *speed(enum usb_device_speed speed)
{
    switch (speed) {
        case USB_SPEED_LOW:
            return "low";
        case USB_SPEED_FULL:
            return "full";
        case USB_SPEED_HIGH:
            return "high";
        default:
            return "unknown speed";
    }
}
static int scnprint_id(struct usb_device *udev, char *buffer, size_t size)
{
    return scnprintf(buffer, size, "%04hx:%04hx v%04hx %s",
                     le16_to_cpu(udev->descriptor.idVendor),
                     le16_to_cpu(udev->descriptor.idProduct),
                     get_bcdDevice(udev),
                     speed(udev->speed));
}


static void print_id(struct usb_device *udev)
{
    char buffer[40];
    
    scnprint_id(udev, buffer, sizeof(buffer));
    buffer[sizeof(buffer)-1] = 0;
    pr_info( "%s\n", buffer);
}


static int ubertooth_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
    int r;
    struct usb_device *udev = interface_to_usbdev(intf);
    
    pr_info("### PROBE\n");
    
    if (id->driver_info == DEVICE_UBERTOOTH_BOOTLOADER) {
        dev_err(&intf->dev, "Bootloader mode not yet supported\n");
        r = -ENODEV;
        goto error;
    } else if (id->driver_info == DEVICE_UBERTOOTH_UNKNOWN) {
        dev_err(&intf->dev, "Model not yet supported\n");
        r = -ENODEV;
        goto error;
    }
        
    print_id(udev);
    
    return 0;
    
error:
    return r;
    
#if 0  
    int r;
    struct usb_device *udev = interface_to_usbdev(intf);
    struct zd_usb *usb;
    struct ieee80211_hw *hw = NULL;
    
    print_id(udev);
    
    if (id->driver_info & DEVICE_INSTALLER)
        return eject_installer(intf);
    
    switch (udev->speed) {
        case USB_SPEED_LOW:
        case USB_SPEED_FULL:
        case USB_SPEED_HIGH:
            break;
        default:
            dev_dbg_f(&intf->dev, "Unknown USB speed\n");
            r = -ENODEV;
            goto error;
    }

r = usb_reset_device(udev);
if (r) {
    dev_err(&intf->dev,
    "couldn't reset usb device. Error number %d\n", r);
    goto error;
}

hw = zd_mac_alloc_hw(intf);
if (hw == NULL) {
    r = -ENOMEM;
    goto error;
}

usb = &zd_hw_mac(hw)->chip.usb;
usb->is_zd1211b = (id->driver_info == DEVICE_ZD1211B) != 0;

r = zd_mac_preinit_hw(hw);
if (r) {
    dev_dbg_f(&intf->dev,
    "couldn't initialize mac. Error number %d\n", r);
    goto error;
}

r = ieee80211_register_hw(hw);
if (r) {
    dev_dbg_f(&intf->dev,
    "couldn't register device. Error number %d\n", r);
    goto error;
}

dev_dbg_f(&intf->dev, "successful\n");
dev_info(&intf->dev, "%s\n", wiphy_name(hw->wiphy));
return 0;
error:
usb_reset_device(interface_to_usbdev(intf));
if (hw) {
    zd_mac_clear(zd_hw_mac(hw));
    ieee80211_free_hw(hw);
}
return r;
#endif  

}

static void ubertooth_disconnect(struct usb_interface *intf)
{
    dev_dbg(&intf->dev, "disconnected\n");
    #if 0
    struct ieee80211_hw *hw = zd_intf_to_hw(intf);
    struct zd_mac *mac;
    struct zd_usb *usb;
    
    /* Either something really bad happened, or we're just dealing with
     * a DEVICE_INSTALLER. */
    if (hw == NULL)
        return;
    
    mac = zd_hw_mac(hw);
    usb = &mac->chip.usb;
    
    dev_dbg_f(zd_usb_dev(usb), "\n");
    
    ieee80211_unregister_hw(hw);
    
    /* Just in case something has gone wrong! */
    zd_usb_disable_tx(usb);
    zd_usb_disable_rx(usb);
    zd_usb_disable_int(usb);
    
    /* If the disconnect has been caused by a removal of the
     * driver module, the reset allows reloading of the driver. If the
     * reset will not be executed here, the upload of the firmware in the
     * probe function caused by the reloading of the driver will fail.
     */
    usb_reset_device(interface_to_usbdev(intf));
    
    zd_mac_clear(mac);
    ieee80211_free_hw(hw);
    dev_dbg(&intf->dev, "disconnected\n");
    #endif  
}


static int ubertooth_pre_reset(struct usb_interface *intf)
{
    #if 0  
    struct ieee80211_hw *hw = usb_get_intfdata(intf);
    struct zd_mac *mac;
    struct zd_usb *usb;
    
    if (!hw || intf->condition != USB_INTERFACE_BOUND)
        return 0;
    
    mac = zd_hw_mac(hw);
    usb = &mac->chip.usb;
    
    usb->was_running = test_bit(ZD_DEVICE_RUNNING, &mac->flags);
    
    zd_usb_stop(usb);
    
    mutex_lock(&mac->chip.mutex);
    #endif  
    return 0;
}

static int ubertooth_post_reset(struct usb_interface *intf)
{
    #if 0  
    struct ieee80211_hw *hw = usb_get_intfdata(intf);
    struct zd_mac *mac;
    struct zd_usb *usb;
    
    if (!hw || intf->condition != USB_INTERFACE_BOUND)
        return 0;
    
    mac = zd_hw_mac(hw);
    usb = &mac->chip.usb;
    
    mutex_unlock(&mac->chip.mutex);
    
    if (usb->was_running)
        zd_usb_resume(usb);
    #endif
    return 0;
}


static struct usb_driver driver = {
    .name           = KBUILD_MODNAME,
    .id_table       = usb_ids,
    .probe          = ubertooth_probe,
    .disconnect     = ubertooth_disconnect,
    .pre_reset      = ubertooth_pre_reset,
    .post_reset     = ubertooth_post_reset,
    .disable_hub_initiated_lpm = 1,
};

static int __init uberdrv_init(void)
{
    int r;
    pr_info(KBUILD_MODNAME" Driver for Ubertooth one\n");
    r = usb_register(&driver);
    if (r) {
        printk(KERN_ERR "%s usb_register() failed. Error number %d\n",
               driver.name, r);
        return r;
    } 
    return 0;
}
static void __exit uberdrv_exit(void)
{
    usb_deregister(&driver);
    pr_info(KBUILD_MODNAME" Driver for Ubertooth one [EXIT]\n");
}
module_init(uberdrv_init);
module_exit(uberdrv_exit);
