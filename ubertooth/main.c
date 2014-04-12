#include <linux/init.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>



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

MODULE_DEVICE_TABLE(usb, usb_ids);

struct uber_usb_dev {
	struct usb_interface* pInterfase;
	struct usb_device* pDevice;
	struct usb_endpoint_descriptor *bulk_in_ep;
	int bulk_in_size;
	char* bulk_in_buffer;
	struct usb_endpoint_descriptor *bulk_out_ep;
	int bulk_out_size;
	char* bulk_out_buffer;

	struct usb_endpoint_descriptor *ctrl_ep;
};


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
    int r,i;
	struct usb_endpoint_descriptor *endpoint;
	struct uber_usb_dev *dev = NULL;
    struct usb_host_interface *iface_desc;
    struct usb_device *udev = interface_to_usbdev(intf);
    
    pr_info("### PROBE\n");
    
    if (id->driver_info == DEVICE_UBERTOOTH_BOOTLOADER) {
        dev_err(&intf->dev, "Bootloader mode not yet supported\n");
        r = -ENODEV;
        goto exit;
    } else if (id->driver_info == DEVICE_UBERTOOTH_UNKNOWN) {
        dev_err(&intf->dev, "Model not yet supported\n");
        r = -ENODEV;
        goto exit;
    }
        
    print_id(udev);

    dev = kzalloc(sizeof(struct uber_usb_dev), GFP_KERNEL);
    if (! dev)
    {
        dev_err(&intf->dev, "cannot allocate memory for struct uber_usb_dev\n");
        r = -ENOMEM;
        goto exit;
    }	

	dev->pDevice = udev;
	dev->pInterfase = intf;

	iface_desc = intf->cur_altsetting;

	for (i=0; i < iface_desc->desc.bNumEndpoints; i++)
	{
        endpoint = &iface_desc->endpoint[i].desc;

        if (!dev->bulk_in_ep && ((endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN)
                && ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
                    USB_ENDPOINT_XFER_BULK)) {
            dev->bulk_in_ep = endpoint;	
			dev->bulk_in_size = endpoint->wMaxPacketSize;
			dev->bulk_in_buffer = kmalloc(dev->bulk_in_size, GFP_KERNEL);
			if (dev->bulk_in_buffer == NULL) {
				dev_err(&intf->dev, "cannot allocate memory for bulk_in_buffer\n");
				r = -ENOMEM;
				goto exit;
			}
        }

		if (!dev->bulk_out_ep && ((endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_OUT)
				&& ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
					USB_ENDPOINT_XFER_BULK)) {
			dev->bulk_out_ep = endpoint; 	
			dev->bulk_out_size = endpoint->wMaxPacketSize;
			dev->bulk_out_buffer = kmalloc(dev->bulk_out_size, GFP_KERNEL);
			if (dev->bulk_out_buffer == NULL) {
				dev_err(&intf->dev, "cannot allocate memory for bulk_out_buffer\n");
				r = -ENOMEM;
				goto exit;
			}
		}

	}

	
	if (dev->bulk_in_ep == NULL || dev->bulk_out_ep == NULL) {
		dev_err(&intf->dev, "Could not find all EP\n");
		r = -ENODEV;
		goto exit;
	}

	usb_set_intfdata(intf, dev);

	usb_get_dev(udev);

	//usb_register_dev(interface, &ml_class);

	
    return 0;
    
exit:
	if (dev != NULL) {
		if (dev->bulk_out_buffer != NULL) {
			kfree(dev->bulk_out_buffer);
		}
		if (dev->bulk_in_buffer != NULL) {
			kfree(dev->bulk_in_buffer);
		}
		kfree(dev);
	}
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
//TODO: do I need these?     
//    .pre_reset      = ubertooth_pre_reset,
//    .post_reset     = ubertooth_post_reset,
//    .disable_hub_initiated_lpm = 1, FOR KERNEL 3.8.x
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

MODULE_AUTHOR("Michael Malyshev");
MODULE_VERSION("1.0");  
MODULE_DESCRIPTION("Network interface driver for Ubertooth One USB Bluetoth sniffer");  
MODULE_LICENSE("GPL");
