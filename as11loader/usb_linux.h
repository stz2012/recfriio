/* public domain */

#ifndef USB_LINUX_H
#define USB_LINUX_H

#include <usb.h>

struct usb_device* get_device( int idvendor, int idproduct )
{
    struct usb_bus *ubus;
    struct usb_device *udev;

    fprintf( stderr, "searching device (vendor = 0x%x, product = 0x%x)...\n", idvendor, idproduct );

    ubus = usb_get_busses();
    while( ubus ) {

        udev = ubus->devices;
        while( udev ){

            fprintf( stderr, "vendor = %x, product = %x\n",
                     udev->descriptor.idVendor, udev->descriptor.idProduct );

            if( udev->descriptor.idVendor == idvendor
                && udev->descriptor.idProduct == idproduct ){
                fprintf( stderr, "found device\n" );
                return udev;
            }

            udev = udev->next;
        }

        ubus = ubus->next;
    }

    fprintf( stderr, "could not find device\n" );
    return NULL;
}


void close_device( usb_dev_handle* uhnd )
{
    if( !uhnd ) return;

    fprintf( stderr, "closing device..." );

    usb_release_interface( uhnd, 0);
    usb_close( uhnd );

    fprintf( stderr, "done\n" );
}


usb_dev_handle* init_device( int idvendor, int idproduct )
{
    struct usb_device *udev = NULL;
    usb_dev_handle *uhnd = NULL;

    fprintf( stderr, "initializing device...\n" );

    usb_init();
    usb_find_busses();
    usb_find_devices();

    if( !( udev = get_device( idvendor, idproduct ) ) ) goto ERROR;

    if( !( uhnd = usb_open( udev ) ) ){
        fprintf( stderr, "could not open device\n" );
        goto ERROR;
    }

    if( usb_set_configuration( uhnd, udev->config->bConfigurationValue ) < 0 ){

        if( usb_detach_kernel_driver_np( uhnd, udev->config->interface->altsetting->bInterfaceNumber ) < 0 ){
            fprintf( stderr, "failed to usb_detach_kernel_driver_np : %s\n", usb_strerror() );
            goto ERROR;
        }

        if( usb_set_configuration( uhnd, udev->config->bConfigurationValue ) < 0 ){
            fprintf( stderr, "failed to set configuration : %s \n", usb_strerror() );;
            goto ERROR;
        }
    }

    if( usb_claim_interface( uhnd, udev->config->interface->altsetting->bInterfaceNumber ) < 0 ){

        if( usb_detach_kernel_driver_np( uhnd, udev->config->interface->altsetting->bInterfaceNumber ) < 0 ){
            fprintf( stderr, "failed to usb_detach_kernel_driver_np : %s\n", usb_strerror() );
            goto ERROR;
        }

        if( usb_claim_interface( uhnd, udev->config->interface->altsetting->bInterfaceNumber ) < 0 ){
            fprintf( stderr, "failed to claim interface : %s\n\n", usb_strerror() );
            goto ERROR;
        }
    }

    return uhnd;

  ERROR:

    close_device( uhnd );
    return NULL;
}


int
send_ctl(uint8_t *data, usb_dev_handle* uhnd, int req, int val, int idx, int len)
{
//    fprintf( stderr, "sending message req = 0x%x, val = 0x%x, idx = 0x%x\n", req, val, idx );

    int ret = usb_control_msg( uhnd, USB_TYPE_VENDOR | USB_ENDPOINT_IN, req, val, idx, data, len, 5000 );
    if( ret != len ){
        fprintf( stderr, "failed to send message : %s\n",  usb_strerror() );
        fprintf( stderr, "req = 0x%x, val = 0x%x, idx = 0x%x\n", req, val, idx );
        ret = 0;
    }

    return ret;
}


int
send_firm( usb_dev_handle* uhnd, int req, int16_t idx, uint8_t *data, int off, int size)
{
    fprintf( stderr, "sending firm offset = 0x%x, size = 0x%x\n", off, size );

    int ret = usb_control_msg( uhnd, USB_TYPE_VENDOR | USB_ENDPOINT_OUT, req, off, idx, data + off, size, 5000 );
    if( ret != size ){
        fprintf( stderr, "failed to send firm : %s\n",  usb_strerror() );
        ret = 0;
    }

    return ret;
}


#endif
