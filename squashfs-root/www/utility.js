var msg_invalid_ip = "<%9%>\n";
function isIPaddr(addr) {
    var i;
    var a;
    if(addr.split) {	
        a = addr.split(".");
    }else {	
        a = cdisplit(addr,".");
    }
    if(a.length != 4) {
        return false;
    }
    for(i = 0; i<a.length; i++) {
        var x = a[i];
        if( x == null || x == "" || !_isNumeric(x) || x<0 || x>255 ) {
            return false;
        }
    }
    return true;
}

function isValidIPaddr(addr) {
    var i;
    var a;
    if(addr.split) {
        a = addr.split(".");
    }else {
        a = cdisplit(addr,".");
    }
    if(a.length != 4) {
        return false;
    }
    for(i = 0; i<a.length; i++) {
        var x = a[i];
        if( x == null || x == "" || !_isNumeric(x) || x<0 || x>255 ) {
            return false;
        }
    }
    
    var ip1 = new String("");
    var ip2 = new String("");
    var ip3 = new String("");
    var ip4 = new String("");
    ip1.value = a[0];
    ip2.value = a[1];
    ip3.value = a[2];
    ip4.value = a[3];
    
    if(checkIP(ip1,ip2,ip3,ip4,254)||(parseInt(ip4.value,10)==0))
        return false;
    
    return true;
}

function isNetmask(mask1,mask2,mask3,mask4)
{
    var netMask;
    var i;
    var bit;
    var isChanged = 0;

    netMask = (mask1.value << 24) | (mask2.value << 16) | (mask3.value << 8) | (mask4.value);
    
    /* Check most byte (must be 255) and least significant bit (must be 0) */
    bit = (netMask & 0xFF000000) >>> 24;
    if (bit != 255)
        return false;
    
    bit = netMask & 1;
    if (bit != 0)
        return false;

    /* Now make sure the bit pattern changes from 0 to 1 only once */
    for (i=1; i<31; i++)
    {
        netMask = netMask >>> 1;
        bit = netMask & 1;

        if (bit != 0)
        {
            if (isChanged == 0)
                isChanged = 1;
        }
        else
        {
            if (isChanged == 1)
                return false;
        }
    }

    return true;
}

function trim(vString)
{ 
	var tString = vString;

	//trim left spaces
	if (tString.charAt(0) == " ")
		tString = trim(tString.substring(1, tString.length));

	//trim right spaces
	if (tString.charAt(tString.length-1) == " ")
		tString = trim(tString.substring(0, tString.length-1))

	return(tString);
}
function isNumOnly(vString)
{
	var NUMBERS = "0123456789";
	var valid = true;
	for(var i=0;i<vString.length;i++)
		if(NUMBERS.indexOf(vString.charAt(i))<0)
			valid = false;
			
	return(valid);
}
function isNull(vField) {
	var ret = false
	vField.value = trim(vField.value)
	
	if (vField.value.length == 0)
		ret = true
	return(ret)
}

function _isNumeric(str) {
    var i;

    if(str.length == 0 || str == null || str == "" || !isDecimalNumber(str)) {
        return false;
    }

    for(i = 0; i<str.length; i++) {
        var c = str.substring(i, i+1);
        if("0" <= c && c <= "9") {
            continue;
        }
        return false;
    }
    return true;
}

function isHex(str) {
    var i;
    for(i = 0; i<str.length; i++) {
        var c = str.substring(i, i+1);
        if(("0" <= c && c <= "9") || ("a" <= c && c <= "f") || ("A" <= c && c <= "F")) {
            continue;
        }
        return false;
    }
    return true;
}

/* Just check if MAC address is all "F", or all "0" , with  ':' in it or not weal @ Aug 14*/ 
function MacStrallf(mac) {
	var temp=mac.value;

    for(i=0; i<mac.value.length;i++) {
        var c = mac.value.substring(i, i+1)
        if (c == "f" || c == "F" || c == "0" || c == ":" || c == "-")
            continue;
		else
			break;
	}
	if (i == mac.value.length)
		return true;
	else
		return false;
}
function checkMacStr(mac) {
    if((mac.value.indexOf(':') != -1)||(mac.value.indexOf('-') != -1))
    {
        mac.value = mac.value.replace(/:/g,"");
        mac.value = mac.value.replace(/-/g,"");
    }
    var temp=mac.value;
    if(mac.value.length != 12) {
        if (mac.focus)
            mac.focus();
        return true;
    }
    for(i=0; i<mac.value.length;i++) {
        var c = mac.value.substring(i, i+1);
        if(("0" <= c && c <= "9") || ("a" <= c && c <= "f") || ("A" <= c && c <= "F")) {
            continue;
        }
        if (mac.focus)
            mac.focus();
        return true;
    }

    if(checkMulticastMAC(mac) || MacStrallf(mac))
    {
        if (mac.focus)
            mac.focus();
        return true;
    }

    mac.value = temp.toUpperCase();
    return false;
}
/* Check Mac Address Format*/
function checkMacMain(mac) {
    if(mac.value.length == 0) {
        if (mac.focus)
            mac.focus();
        return true;
    }
    for(i=0; i<mac.value.length;i++) {
        var c = mac.value.substring(i, i+1)
        if(("0" <= c && c <= "9") || ("a" <= c && c <= "f") || ("A" <= c && c <= "F")) {
            continue;
        }
        if (mac.focus)
            mac.focus();
        return true;
    }
    if(mac.value.length == 1) {
        mac.value = "0"+mac.value;
    }
    mac.value = mac.value.toUpperCase();
    return false;
}
function checkMacAddress(mac1, mac2, mac3, mac4, mac5, mac6) {
    if(checkMacMain(mac1)) return true;
    if(checkMacMain(mac2)) return true;
    if(checkMacMain(mac3)) return true;
    if(checkMacMain(mac4)) return true;
    if(checkMacMain(mac5)) return true;
    if(checkMacMain(mac6)) return true;
    var mac_str = new String("");
    mac_str.value =  mac1.value + mac2.value + mac3.value
                 + mac4.value + mac5.value + mac6.value;
    if(checkMulticastMAC(mac_str) || MacStrallf(mac_str))
        return true;
    
    return false;
}

/* Check Multicast MAC */
function checkMulticastMAC(macaddr) {
    var mac_defined = macaddr.value;
    var macadr_first_byte = parseInt(mac_defined.substring(0,2),16);
    var Multicast_Flag = macadr_first_byte & 0x01;
    if( Multicast_Flag )
        return true;
    return false;
}

/* Check IP Address Format*/
function checkIPMain(ip,max) {
    if( isNumeric(ip, max) ) {
        if (ip.focus)
            ip.focus();
        return true;
    }
}

function checkIP(ip1, ip2, ip3, ip4,max) {
    if(checkIPMain(ip1,255)) return true; 
    if(checkIPMain(ip2,255)) return true;
    if(checkIPMain(ip3,255)) return true;
    if(checkIPMain(ip4,max)) return true;

    if((ip1.value.charAt(0)=="0" && ip1.value.length!=1) ||
       (ip2.value.charAt(0)=="0" && ip2.value.length!=1) ||
       (ip3.value.charAt(0)=="0" && ip3.value.length!=1) ||
       (ip4.value.charAt(0)=="0" && ip4.value.length!=1)) 
        return true; 

    if((parseInt(ip1.value)==0)||
       ((parseInt(ip1.value)==0)&&(parseInt(ip2.value)==0)&&
        (parseInt(ip3.value)==0)&&(parseInt(ip4.value)==0)))
        return true;

    var loopback_ip_start   = (127 << 24)  |   (0 << 16)   |   (0 << 8)   |   (0);
    var loopback_ip_end     = (127 << 24)  |  (255 << 16)  |  (255 << 8)  | (255);
    var groupcast_ip_start  = (224 << 24)  |   (0 << 16)   |   (0 << 8)   |   (0);
    var groupcast_ip_end    = (239 << 24)  |  (255 << 16)  |  (255 << 8)  | (255);
    var dhcpresv_ip_start   = (169 << 24)  |  (254 << 16)  |   (0 << 8)   |   (0);
    var dhcpresv_ip_end     = (169 << 24)  |  (254 << 16)  |  (255 << 8)  | (255);
    var ip_addr = (ip1.value << 24) | (ip2.value << 16) | (ip3.value << 8) | (ip4.value);
    if((ip_addr >= loopback_ip_start)&&(ip_addr <= loopback_ip_end))
        return true;
    if((ip_addr >= groupcast_ip_start)&&(ip_addr <= groupcast_ip_end ))
        return true;
    if((ip_addr >= dhcpresv_ip_start)&&(ip_addr <= dhcpresv_ip_end))
        return true;

    return false;
}

function checkIPMatchWithNetmask(mask1,mask2,mask3,mask4,ip1, ip2, ip3, ip4)
{
    var netMask;
	var ipAddr;

    netMask = (mask1.value << 24) | (mask2.value << 16) | (mask3.value << 8) | (mask4.value);
    ipAddr = (ip1.value << 24) | (ip2.value << 16) | (ip3.value << 8) | (ip4.value);

    if (((~netMask)&ipAddr) == 0)
		return true;
	if (((~netMask)&ipAddr) == (~netMask))
		return true;
	return false;
}

function isDecimalNumber(str)
{
    if ((str.charAt(0)=='0') && (str.length != 1))
         return false;
    for(var i=0;i<str.length;i++)
    {
         if(str.charAt(i)<'0'||str.charAt(i)>'9')
         return false;
    }
    return true;
}

/* Check Numeric*/
function isNumeric(str, max) {
    if(str.value.length == 0 || str.value == null || str.value == "") {
        if (str.focus)
            str.focus();
        return true;
    }
    
    var i = parseInt(str.value,10);
    if(i>max) {
        if (str.focus)
            str.focus();
        return true;
    }
    for(i=0; i<str.value.length; i++) {
        var c = str.value.substring(i, i+1);
        if("0" <= c && c <= "9") {
            continue;
        }
        if (str.focus)
            str.focus();
        return true;
    }
    return false;
}

/* Check Blank*/
function isBlank(str) {
    if(str.value == "") {
        if (str.focus)
            str.focus();
        return true;
    } else 
        return false;
}

/* Check Phone Number*/
function isPhonenum(str) {
    var i;
    if(str.value.length == 0) {
        if (str.focus)
            str.focus();
        return true;
    }
    for (i = 0; i<str.value.length; i++) {
        var c = str.value.substring(i, i+1);
        if (c>= "0" && c <= "9")
            continue;
        if ( c == '-' && i !=0 && i != (str.value.length-1) )
            continue;
        if ( c == ',' ) continue;
        if (c == ' ') continue;
        if (c>= 'A' && c <= 'Z') continue;
        if (c>= 'a' && c <= 'z') continue;
        if (str.focus)
            str.focus();
        return true;
    }
    return false;
}

/* 0:close 1:open*/
function openHelpWindow(filename) {
    helpWindow = window.open(filename,"thewindow","width=300,height=400,scrollbars=yes,resizable=yes,menubar=no");
}

function alertPassword(formObj) {
    if (formObj.focus)
        formObj.focus();
}
function isEqual(cp1,cp2)
{
	if(parseInt(cp1.value,10) == parseInt(cp2.value,10))
	{
		if (cp2.focus)
			cp2.focus();
		return true;
	}	
	else return false;
}
function setDisabled(OnOffFlag,formFields)
{
	for (var i = 1; i < setDisabled.arguments.length; i++)
		setDisabled.arguments[i].disabled = OnOffFlag;
}

function cp_ip(from1,from2,from3,from4,to1,to2,to3,to4)
//true invalid from and to ip;  false valid from and to ip;
{
    var total1 = 0;
    var total2 = 0;
    
    total1 += parseInt(from4.value,10);
    total1 += parseInt(from3.value,10)*256;
    total1 += parseInt(from2.value,10)*256*256;
    total1 += parseInt(from1.value,10)*256*256*256;
    
    total2 += parseInt(to4.value,10);
    total2 += parseInt(to3.value,10)*256;
    total2 += parseInt(to2.value,10)*256*256;
    total2 += parseInt(to1.value,10)*256*256*256;
    if(total1 > total2)
        return true;
    return false;
}
function pi(val)
{
    return parseInt(val,10);
}    
function alertR(str)    
{
    alert(str);
    return false;
}    
    