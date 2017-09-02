//
//  main.cpp
//  WakeMeUp
//
//  Created by Benedek Tomka on 2017. 08. 30..
//
//  Copyright 2017 Benedek Tomka
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
//  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <iostream>

#include <IOKit/IOKitLib.h>
#include <IOKit/IOReturn.h>
#include <IOKit/IOMessage.h>
#include <IOKit/IOCFPlugIn.h>


#include <CoreFoundation/CFNumber.h>
#include <CoreFoundation/CFDictionary.h>

#include <mach/mach.h>

#include <IOKit/firewire/IOFireWireLib.h>


IONotificationPortRef   gNotifyPort;
CFRunLoopSourceRef      gNotifySource;

void fw410Callback(void* nullable, io_iterator_t iterator);


// uninteresting converter function no.0
void			FillBlockBig(Ptr buf, UInt32 size, CFStringRef word)
{
    char tempString[84] ;
    CFStringGetCString(word, tempString, 84, kCFStringEncodingASCII );
    
    for(int i = 0, ptr = 0; ptr<size; i=i+2, ptr++) {
        UInt32 high = strtol((char[]){tempString[i], 0}, NULL, 16) * 16;
        UInt32 low = strtol((char[]){tempString[i+1], 0}, NULL, 16);
        
        ((char*)buf)[ptr] = (char)high+low;
        
    }
}

// uninteresting converter function no.1
void CFStringGetFWAddress( const CFStringRef string, FWAddress & outAddr )
{
    CFArrayRef 				tempArray = ::CFStringCreateArrayBySeparatingStrings( kCFAllocatorDefault, string, CFSTR(".") ) ;
    
    CFStringRef 			addressCFString	= ::CFStringCreateByCombiningStrings( kCFAllocatorDefault, tempArray, CFSTR("") ) ;
    
    ::CFRelease( tempArray ) ;
    
    if ( kCFCompareEqualTo == ::CFStringCompare( addressCFString, CFSTR("random"),  kCFCompareCaseInsensitive | kCFCompareNonliteral ) )
    {
        outAddr = FWAddress(random(), random(), 0) ;
        return ;
    }
    
    // convert address to a number
    char tempString[42] ;
    CFStringGetCString( addressCFString, tempString, 42, kCFStringEncodingASCII);
    
    UInt64	address = strtouq( tempString, nil, 16 ) ;
    
    outAddr = FWAddress( (address >> 32) & 0xFFFF, address & 0xFFFFFFFF, (address >> 48) & 0xFFFF ) ;
}

int main(int argc, const char * argv[]) {
    std::cout << "Hello, World!\n";
    
    
    kern_return_t   result;
    mach_port_t     masterPort;
    
    result = IOMasterPort(MACH_PORT_NULL, &masterPort );
    
    CFMutableDictionaryRef  matchingDictionary = IOServiceMatching("IOFireWireDevice");
    
    // use the vendor name to select devices
    CFStringRef str = CFStringCreateWithCString(NULL, "M-AUDIO", CFStringGetSystemEncoding());
    CFDictionaryAddValue( matchingDictionary, CFSTR("FireWire Vendor Name"), str);
    CFRelease(str);
    

    
    io_iterator_t   iterator;
    
    gNotifyPort = IONotificationPortCreate( masterPort );
    gNotifySource = IONotificationPortGetRunLoopSource( gNotifyPort );
    CFRunLoopAddSource( CFRunLoopGetCurrent(), gNotifySource,
                       kCFRunLoopDefaultMode );
    
    std::cout << "Looking for suitable interfaces... (Ctrl-Z to exit)\n";
    // get notifications for added devices
    result = IOServiceAddMatchingNotification(gNotifyPort, kIOFirstMatchNotification, (CFDictionaryRef)matchingDictionary, fw410Callback, NULL, &iterator);
    
    // do a turn on the connected devices
    fw410Callback(NULL, iterator);
    
    CFRunLoopRun();
    
    return 0;
}

// this is where the magic happens
void fw410Callback(void* nullable, io_iterator_t iterator) {
    io_object_t aDevice;
    
    while ( (aDevice = IOIteratorNext( iterator ) ) != 0 ) {
        std::cout << "I have found one. Lets wake it up...\n";
        
        IOCFPlugInInterface**   cfPlugInInterface = 0;
        IOReturn                result;
        SInt32                  theScore;
        
        result = IOCreatePlugInInterfaceForService( aDevice, kIOFireWireLibTypeID,
                                                   kIOCFPlugInInterfaceID, &cfPlugInInterface, &theScore );
        
        IOFireWireLibDeviceRef  fwDeviceInterface = 0;
        
        (*cfPlugInInterface)->QueryInterface(cfPlugInInterface, CFUUIDGetUUIDBytes(kIOFireWireDeviceInterfaceID_v5 ), (void**)&fwDeviceInterface);
        
        
        UInt32 size = 12; // the magic happens to be 12 bytes
        
        // convert string to fwaddress
        FWAddress addr;
        CFStringGetFWAddress(CFStringCreateWithCString(NULL, "0xFFFFC8021000", CFStringGetSystemEncoding()), addr ) ;
        
        // convert the string to a suitable byte array
        Ptr	buf 	= new char[size];
        FillBlockBig(buf, size, CFStringCreateWithCString(NULL, "010000000000110100000000", CFStringGetSystemEncoding()));
        
        // open device for access (exclusive)
        (**fwDeviceInterface).Open(fwDeviceInterface);
        
        UInt32 generation;
        (**fwDeviceInterface).GetBusGeneration(fwDeviceInterface, &generation);
        
        // write out the buffer
        IOReturn returnv = (**fwDeviceInterface).Write(fwDeviceInterface, aDevice, &addr, buf, &size, kFWFailOnReset, generation) ;
        
        // send a bus reset to notify the system
        (**fwDeviceInterface).BusReset(fwDeviceInterface);
        
        
        (**fwDeviceInterface).Close(fwDeviceInterface);
        
        
        std::cout << "Magic is done. Lets pray!\n\n";
        if(returnv == 0) {
            std::cout << "It looks like the device is enabled. :) \nYou can try to load the driver kext!\n";
        } else {
            std::cout << "The program wasn't able to enable the device. Contact the developer.\n";
        }
        printf(")\n\nWritten by \033[1mBenedek Tomka\033[0m,\n\tusing the research of \033[1mTakashi Sakamoto\033[0m \n\t(from the linux FFADO community).\n");
        
        exit(0);
    }
    
    
}

