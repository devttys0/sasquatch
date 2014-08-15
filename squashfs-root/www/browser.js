function isIE(){
    var browser = new Object();
    browser.version = parseInt(navigator.appVersion);
    browser.isNs = false;
    browser.isIe = false;
    if(navigator.appName.indexOf("Netscape") != -1)
        browser.isNs = true;
    else if(navigator.appName.indexOf("Microsoft") != -1)
        browser.isIe = true;
    if(browser.isNs) return false;
    else if (browser.isIe) return true;
}

function add(out,in1,in2,in3,in4) {
    var Total;
    Total=in1.value+"."+in2.value+"."+in3.value+"."+in4.value;
    out.value=Total; 
}

function load4(Mydata,ip1,ip2,ip3,ip4) {
    var len; var ad; var temp;
    var Myall;
    Myall=Mydata.value;    //ip1 
    len=Myall.length;
    temp=Myall.indexOf(".");
    ad=Myall.substring(0,temp); 
    ip1.value=ad;
    //ip2 
    Myall=Myall.substring(temp+1,len);
    len=Myall.length;
    temp=Myall.indexOf(".");
    ad=Myall.substring(0,temp);
    ip2.value=ad;
    //ip3 
    Myall=Myall.substring(temp+1,len);
    len=Myall.length;
    temp=Myall.indexOf(".");
    ad=Myall.substring(0,temp);
    ip3.value=ad;
    //ip4 
    Myall=Myall.substring(temp+1,len);
    ad=Myall; ip4.value=ad;
} 