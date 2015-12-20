// YouTube - EZ Download
// Version 0.4.8 BETA
// 2013-03-20
// Copyright (c) 2010-2012, Byron Rogers
// Modified by Fabien Coeurjoly for Odyssey Web Browser.
// Released under the GPL license
// http://www.gnu.org/copyleft/gpl.html
//
// --------------------------------------------------------------------
//
// This is a Greasemonkey user script.
//
// To install, you need Greasemonkey: http://www.greasespot.net/
// Then restart Firefox and revisit this script.
// Under Tools, there will be a new menu item to "Install User Script".
// Accept the default configuration and install.
//
// To uninstall, go to Tools/Manage User Scripts,
// select "YouTube - EZ Download", and click Uninstall.
//
// --------------------------------------------------------------------
//
// ==UserScript==
// @name			YouTube - EZ Download
// @namespace		http://www.Daem0nX.com (Daem0nX@gmail.com)
// @description		Adds menu (bottom right) with download links for available formats.
// @version			0.4.8
// @notes			2013.03.20 - 0.4.8<br/>Added support to check if the download link is valid on click.<br/>^If the download link has expired a message to refresh will be displayed.<br/>Fixed broken download links due to YouTube code changes.<br/>Known issue in Chrome - Clicking a download link will sometimes make the video restart or freeze.<br/><br/>2012.12.21 - 0.4.7<br/>Fixed broken download links due to YouTube code changes.<br/>Refactored code that parses download links - completely rebuilds link with expected querystring variables.<br/><br/>2012.12.17 - 0.4.6<br/>Fixed username (code %U) using incorrect value if logged into YouTube.<br/><br/>2012.12.06 - 0.4.5<br/>Fixed username and title (code %U and %T) not working due to YouTube code changes.<br/>Modified script meta data.<br/>^Added @grant for GM_getValue, GM_setValue, GM_deleteValue, GM_addStyle, GM_xmlhttpRequest, GM_log.<br/>Upgraded jQuery from 1.8.2 to 1.8.3.<br/><br/>2012.09.22 - 0.4.4<br/>Added format info for 17, 36.<br/>Fixed video length 00 seconds showing as 0.<br/>Upgraded jQuery from 1.7.2 to 1.8.2.<br/><br/>2012.09.13 - 0.4.3<br/>Fixed broken download links.<br/><br/>2012.05.05 - 0.4.2<br/>Fixed HTML5 video playback in Chrome.<br/><br/>2012.05.03 - 0.4.1<br/>Added support for HTML5 videos.<br/>Fixed Chrome download breaking currently playing video.<br/>^Download links now open in a new tab.<br/>Fixed username (video title %U) not working due to YouTube code changes.<br/>Fixed menu and overlay border radius not working in Firefox 13 beta.<br/>Upgraded jQuery from 1.6.2 to 1.7.2.<br/>^Changed jQuery calls .bind() and .unbind() to .on() and .off().
// @version		  	$VER: YouTube - EZ Download 0.4.8 (20.03.2013)
// @url			  	http://fabportnawak.free.fr/owb/scripts/Youtube_EZDownload.js
// @include       	http://youtube.com/watch*
// @include			http://*.youtube.com/watch*
// @include       	http://youtube.*/watch*
// @include			http://youtube-nocookie.com/watch*
// @include			http://*.youtube-nocookie.com/watch*
// @include			http://youtube-nocookie.*/watch*
// @include		  	http://*.youtube.com/user/*
// @include       	https://youtube.com/watch*
// @include			https://*.youtube.com/watch*
// @include       	https://youtube.*/watch*
// @include			https://youtube-nocookie.com/watch*
// @include			https://*.youtube-nocookie.com/watch*
// @include			https://youtube-nocookie.*/watch*
// @include		  	https://*.youtube.com/user/*
// @grant			GM_getValue
// @grant			GM_setValue
// @grant			GM_deleteValue
// @grant			GM_addStyle
// @grant			GM_xmlhttpRequest
// @grant			GM_log
// ==/UserScript==

// ========= ADD FROM HERE ONWARDS TO YOUR SCRIPT =========
// Update Notification System
// ========================================================
// === Edit the next four lines to suit your script. ===
var scriptName = 'YouTube - EZ Download';
var scriptId = '87659';
var scriptVersion = '0.4.8';
var scriptUpdateText = '2013.03.20 - 0.4.8<br/>Added support to check if the download link is valid on click.<br/>^If the download link has expired a message to refresh will be displayed.<br/>Fixed broken download links due to YouTube code changes.<br/>Known issue in Chrome - Clicking a download link will sometimes make the video restart or freeze.<br/><br/>2012.12.21 - 0.4.7<br/>Fixed broken download links due to YouTube code changes.<br/>Refactored code that parses download links - completely rebuilds link with expected querystring variables.<br/><br/>2012.12.17 - 0.4.6<br/>Fixed username (code %U) using incorrect value if logged into YouTube.<br/><br/>2012.12.06 - 0.4.5<br/>Fixed username and title (code %U and %T) not working due to YouTube code changes.<br/>Modified script meta data.<br/>^Added @grant for GM_getValue, GM_setValue, GM_deleteValue, GM_addStyle, GM_xmlhttpRequest, GM_log.<br/>Upgraded jQuery from 1.8.2 to 1.8.3.<br/><br/>2012.09.22 - 0.4.4<br/>Added format info for 17, 36.<br/>Fixed video length 00 seconds showing as 0.<br/>Upgraded jQuery from 1.7.2 to 1.8.2.<br/><br/>2012.09.13 - 0.4.3<br/>Fixed broken download links.<br/><br/>2012.05.05 - 0.4.2<br/>Fixed HTML5 video playback in Chrome.<br/><br/>2012.05.03 - 0.4.1<br/>Added support for HTML5 videos.<br/>Fixed Chrome download breaking currently playing video.<br/>^Download links now open in a new tab.<br/>Fixed username (video title %U) not working due to YouTube code changes.<br/>Fixed menu and overlay border radius not working in Firefox 13 beta.<br/>Upgraded jQuery from 1.6.2 to 1.7.2.<br/>^Changed jQuery calls .bind() and .unbind() to .on() and .off().';
// === Stop editing here. ===

var debugUpdate = false;
var debugSettings = false;
var debugGM = false;
var isWebkit = false;
var donationLink_paypal = 'https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=KYUZ6SKY7YQCC';
var donationLink_flattr = 'https://flattr.com/thing/412439/YouTube-EZ-Download';
var videolistRefresh = false;

function GM_addStyle(css) {
	var head, style;
	head = document.getElementsByTagName('head')[0];
	if (!head) { return; }
	style = document.createElement('style');
	style.type = 'text/css';
	try {
		style.innerHTML = css;
	} catch(x) { style.innerText = css; }
	head.appendChild(style);
}

function GM_xmlhttpRequest(details) {
    var xmlhttp = new XMLHttpRequest();
    xmlhttp.onreadystatechange = function() {
        var responseState = {
            responseXML:(xmlhttp.readyState==4 ? xmlhttp.responseXML : ''),
            responseText:(xmlhttp.readyState==4 ? xmlhttp.responseText : ''),
            readyState:xmlhttp.readyState,
            responseHeaders:(xmlhttp.readyState==4 ? xmlhttp.getAllResponseHeaders() : ''),
            status:(xmlhttp.readyState==4 ? xmlhttp.status : 0),
            statusText:(xmlhttp.readyState==4 ? xmlhttp.statusText : '')
        }
        if (details["onreadystatechange"]) {
            details["onreadystatechange"](responseState);
        }
        if (xmlhttp.readyState==4) {
            if (details["onload"] && xmlhttp.status>=200 && xmlhttp.status<300) {
                details["onload"](responseState);
            }
            if (details["onerror"] && (xmlhttp.status<200 || xmlhttp.status>=300)) {
                details["onerror"](responseState);
            }
        }
    }
    try {
      //cannot do cross domain
      xmlhttp.open(details.method, details.url);
    } catch(e) {
      if( details["onerror"] ) {
        //simulate a real error
        details["onerror"]({responseXML:'',responseText:'',readyState:4,responseHeaders:'',status:403,statusText:'Forbidden'});
      }
      return;
    }
    if (details.headers) {
        for (var prop in details.headers) {
            xmlhttp.setRequestHeader(prop, details.headers[prop]);
        }
    }
    xmlhttp.send(null);
}  

GM_getValue = function ( cookieName, oDefault ) {
    var cookieJar = document.cookie.split( "; " );
    for( var x = 0; x < cookieJar.length; x++ ) {
        var oneCookie = cookieJar[x].split( "=" );
        if( oneCookie[0] == escape( cookieName ) ) {
            try {
                var footm = unescape( oneCookie[1] );
            } catch(e) { return oDefault; }
            return footm;
        }
    }
    return oDefault;
};

GM_setValue = function ( cookieName, cookieValue, lifeTime ) {
    if( !cookieName ) { return; }
    if( lifeTime == "delete" ) { lifeTime = -10; } else { lifeTime = 31536000; }
    document.cookie = escape( cookieName ) + "=" + escape( cookieValue ) + ";expires=" + ( new Date( ( new Date() ).getTime() + ( 1000 * lifeTime ) ) ).toGMTString() + ";path=/";
}

function GMlog(text) {
	if (debugGM) {
		//GM_log(text);
		//console.log(text);
		//use unsafeWindow due to Greasemonkey / Firebug issue
		unsafeWindow.console.log(text);
		// or
		//alert(text);
	}
}

/*! jQuery v1.8.3 jquery.com | jquery.org/license */
(function(e,t){function _(e){var t=M[e]={};return v.each(e.split(y),function(e,n){t[n]=!0}),t}function H(e,n,r){if(r===t&&e.nodeType===1){var i="data-"+n.replace(P,"-$1").toLowerCase();r=e.getAttribute(i);if(typeof r=="string"){try{r=r==="true"?!0:r==="false"?!1:r==="null"?null:+r+""===r?+r:D.test(r)?v.parseJSON(r):r}catch(s){}v.data(e,n,r)}else r=t}return r}function B(e){var t;for(t in e){if(t==="data"&&v.isEmptyObject(e[t]))continue;if(t!=="toJSON")return!1}return!0}function et(){return!1}function tt(){return!0}function ut(e){return!e||!e.parentNode||e.parentNode.nodeType===11}function at(e,t){do e=e[t];while(e&&e.nodeType!==1);return e}function ft(e,t,n){t=t||0;if(v.isFunction(t))return v.grep(e,function(e,r){var i=!!t.call(e,r,e);return i===n});if(t.nodeType)return v.grep(e,function(e,r){return e===t===n});if(typeof t=="string"){var r=v.grep(e,function(e){return e.nodeType===1});if(it.test(t))return v.filter(t,r,!n);t=v.filter(t,r)}return v.grep(e,function(e,r){return v.inArray(e,t)>=0===n})}function lt(e){var t=ct.split("|"),n=e.createDocumentFragment();if(n.createElement)while(t.length)n.createElement(t.pop());return n}function Lt(e,t){return e.getElementsByTagName(t)[0]||e.appendChild(e.ownerDocument.createElement(t))}function At(e,t){if(t.nodeType!==1||!v.hasData(e))return;var n,r,i,s=v._data(e),o=v._data(t,s),u=s.events;if(u){delete o.handle,o.events={};for(n in u)for(r=0,i=u[n].length;r<i;r++)v.event.add(t,n,u[n][r])}o.data&&(o.data=v.extend({},o.data))}function Ot(e,t){var n;if(t.nodeType!==1)return;t.clearAttributes&&t.clearAttributes(),t.mergeAttributes&&t.mergeAttributes(e),n=t.nodeName.toLowerCase(),n==="object"?(t.parentNode&&(t.outerHTML=e.outerHTML),v.support.html5Clone&&e.innerHTML&&!v.trim(t.innerHTML)&&(t.innerHTML=e.innerHTML)):n==="input"&&Et.test(e.type)?(t.defaultChecked=t.checked=e.checked,t.value!==e.value&&(t.value=e.value)):n==="option"?t.selected=e.defaultSelected:n==="input"||n==="textarea"?t.defaultValue=e.defaultValue:n==="script"&&t.text!==e.text&&(t.text=e.text),t.removeAttribute(v.expando)}function Mt(e){return typeof e.getElementsByTagName!="undefined"?e.getElementsByTagName("*"):typeof e.querySelectorAll!="undefined"?e.querySelectorAll("*"):[]}function _t(e){Et.test(e.type)&&(e.defaultChecked=e.checked)}function Qt(e,t){if(t in e)return t;var n=t.charAt(0).toUpperCase()+t.slice(1),r=t,i=Jt.length;while(i--){t=Jt[i]+n;if(t in e)return t}return r}function Gt(e,t){return e=t||e,v.css(e,"display")==="none"||!v.contains(e.ownerDocument,e)}function Yt(e,t){var n,r,i=[],s=0,o=e.length;for(;s<o;s++){n=e[s];if(!n.style)continue;i[s]=v._data(n,"olddisplay"),t?(!i[s]&&n.style.display==="none"&&(n.style.display=""),n.style.display===""&&Gt(n)&&(i[s]=v._data(n,"olddisplay",nn(n.nodeName)))):(r=Dt(n,"display"),!i[s]&&r!=="none"&&v._data(n,"olddisplay",r))}for(s=0;s<o;s++){n=e[s];if(!n.style)continue;if(!t||n.style.display==="none"||n.style.display==="")n.style.display=t?i[s]||"":"none"}return e}function Zt(e,t,n){var r=Rt.exec(t);return r?Math.max(0,r[1]-(n||0))+(r[2]||"px"):t}function en(e,t,n,r){var i=n===(r?"border":"content")?4:t==="width"?1:0,s=0;for(;i<4;i+=2)n==="margin"&&(s+=v.css(e,n+$t[i],!0)),r?(n==="content"&&(s-=parseFloat(Dt(e,"padding"+$t[i]))||0),n!=="margin"&&(s-=parseFloat(Dt(e,"border"+$t[i]+"Width"))||0)):(s+=parseFloat(Dt(e,"padding"+$t[i]))||0,n!=="padding"&&(s+=parseFloat(Dt(e,"border"+$t[i]+"Width"))||0));return s}function tn(e,t,n){var r=t==="width"?e.offsetWidth:e.offsetHeight,i=!0,s=v.support.boxSizing&&v.css(e,"boxSizing")==="border-box";if(r<=0||r==null){r=Dt(e,t);if(r<0||r==null)r=e.style[t];if(Ut.test(r))return r;i=s&&(v.support.boxSizingReliable||r===e.style[t]),r=parseFloat(r)||0}return r+en(e,t,n||(s?"border":"content"),i)+"px"}function nn(e){if(Wt[e])return Wt[e];var t=v("<"+e+">").appendTo(i.body),n=t.css("display");t.remove();if(n==="none"||n===""){Pt=i.body.appendChild(Pt||v.extend(i.createElement("iframe"),{frameBorder:0,width:0,height:0}));if(!Ht||!Pt.createElement)Ht=(Pt.contentWindow||Pt.contentDocument).document,Ht.write("<!doctype html><html><body>"),Ht.close();t=Ht.body.appendChild(Ht.createElement(e)),n=Dt(t,"display"),i.body.removeChild(Pt)}return Wt[e]=n,n}function fn(e,t,n,r){var i;if(v.isArray(t))v.each(t,function(t,i){n||sn.test(e)?r(e,i):fn(e+"["+(typeof i=="object"?t:"")+"]",i,n,r)});else if(!n&&v.type(t)==="object")for(i in t)fn(e+"["+i+"]",t[i],n,r);else r(e,t)}function Cn(e){return function(t,n){typeof t!="string"&&(n=t,t="*");var r,i,s,o=t.toLowerCase().split(y),u=0,a=o.length;if(v.isFunction(n))for(;u<a;u++)r=o[u],s=/^\+/.test(r),s&&(r=r.substr(1)||"*"),i=e[r]=e[r]||[],i[s?"unshift":"push"](n)}}function kn(e,n,r,i,s,o){s=s||n.dataTypes[0],o=o||{},o[s]=!0;var u,a=e[s],f=0,l=a?a.length:0,c=e===Sn;for(;f<l&&(c||!u);f++)u=a[f](n,r,i),typeof u=="string"&&(!c||o[u]?u=t:(n.dataTypes.unshift(u),u=kn(e,n,r,i,u,o)));return(c||!u)&&!o["*"]&&(u=kn(e,n,r,i,"*",o)),u}function Ln(e,n){var r,i,s=v.ajaxSettings.flatOptions||{};for(r in n)n[r]!==t&&((s[r]?e:i||(i={}))[r]=n[r]);i&&v.extend(!0,e,i)}function An(e,n,r){var i,s,o,u,a=e.contents,f=e.dataTypes,l=e.responseFields;for(s in l)s in r&&(n[l[s]]=r[s]);while(f[0]==="*")f.shift(),i===t&&(i=e.mimeType||n.getResponseHeader("content-type"));if(i)for(s in a)if(a[s]&&a[s].test(i)){f.unshift(s);break}if(f[0]in r)o=f[0];else{for(s in r){if(!f[0]||e.converters[s+" "+f[0]]){o=s;break}u||(u=s)}o=o||u}if(o)return o!==f[0]&&f.unshift(o),r[o]}function On(e,t){var n,r,i,s,o=e.dataTypes.slice(),u=o[0],a={},f=0;e.dataFilter&&(t=e.dataFilter(t,e.dataType));if(o[1])for(n in e.converters)a[n.toLowerCase()]=e.converters[n];for(;i=o[++f];)if(i!=="*"){if(u!=="*"&&u!==i){n=a[u+" "+i]||a["* "+i];if(!n)for(r in a){s=r.split(" ");if(s[1]===i){n=a[u+" "+s[0]]||a["* "+s[0]];if(n){n===!0?n=a[r]:a[r]!==!0&&(i=s[0],o.splice(f--,0,i));break}}}if(n!==!0)if(n&&e["throws"])t=n(t);else try{t=n(t)}catch(l){return{state:"parsererror",error:n?l:"No conversion from "+u+" to "+i}}}u=i}return{state:"success",data:t}}function Fn(){try{return new e.XMLHttpRequest}catch(t){}}function In(){try{return new e.ActiveXObject("Microsoft.XMLHTTP")}catch(t){}}function $n(){return setTimeout(function(){qn=t},0),qn=v.now()}function Jn(e,t){v.each(t,function(t,n){var r=(Vn[t]||[]).concat(Vn["*"]),i=0,s=r.length;for(;i<s;i++)if(r[i].call(e,t,n))return})}function Kn(e,t,n){var r,i=0,s=0,o=Xn.length,u=v.Deferred().always(function(){delete a.elem}),a=function(){var t=qn||$n(),n=Math.max(0,f.startTime+f.duration-t),r=n/f.duration||0,i=1-r,s=0,o=f.tweens.length;for(;s<o;s++)f.tweens[s].run(i);return u.notifyWith(e,[f,i,n]),i<1&&o?n:(u.resolveWith(e,[f]),!1)},f=u.promise({elem:e,props:v.extend({},t),opts:v.extend(!0,{specialEasing:{}},n),originalProperties:t,originalOptions:n,startTime:qn||$n(),duration:n.duration,tweens:[],createTween:function(t,n,r){var i=v.Tween(e,f.opts,t,n,f.opts.specialEasing[t]||f.opts.easing);return f.tweens.push(i),i},stop:function(t){var n=0,r=t?f.tweens.length:0;for(;n<r;n++)f.tweens[n].run(1);return t?u.resolveWith(e,[f,t]):u.rejectWith(e,[f,t]),this}}),l=f.props;Qn(l,f.opts.specialEasing);for(;i<o;i++){r=Xn[i].call(f,e,l,f.opts);if(r)return r}return Jn(f,l),v.isFunction(f.opts.start)&&f.opts.start.call(e,f),v.fx.timer(v.extend(a,{anim:f,queue:f.opts.queue,elem:e})),f.progress(f.opts.progress).done(f.opts.done,f.opts.complete).fail(f.opts.fail).always(f.opts.always)}function Qn(e,t){var n,r,i,s,o;for(n in e){r=v.camelCase(n),i=t[r],s=e[n],v.isArray(s)&&(i=s[1],s=e[n]=s[0]),n!==r&&(e[r]=s,delete e[n]),o=v.cssHooks[r];if(o&&"expand"in o){s=o.expand(s),delete e[r];for(n in s)n in e||(e[n]=s[n],t[n]=i)}else t[r]=i}}function Gn(e,t,n){var r,i,s,o,u,a,f,l,c,h=this,p=e.style,d={},m=[],g=e.nodeType&&Gt(e);n.queue||(l=v._queueHooks(e,"fx"),l.unqueued==null&&(l.unqueued=0,c=l.empty.fire,l.empty.fire=function(){l.unqueued||c()}),l.unqueued++,h.always(function(){h.always(function(){l.unqueued--,v.queue(e,"fx").length||l.empty.fire()})})),e.nodeType===1&&("height"in t||"width"in t)&&(n.overflow=[p.overflow,p.overflowX,p.overflowY],v.css(e,"display")==="inline"&&v.css(e,"float")==="none"&&(!v.support.inlineBlockNeedsLayout||nn(e.nodeName)==="inline"?p.display="inline-block":p.zoom=1)),n.overflow&&(p.overflow="hidden",v.support.shrinkWrapBlocks||h.done(function(){p.overflow=n.overflow[0],p.overflowX=n.overflow[1],p.overflowY=n.overflow[2]}));for(r in t){s=t[r];if(Un.exec(s)){delete t[r],a=a||s==="toggle";if(s===(g?"hide":"show"))continue;m.push(r)}}o=m.length;if(o){u=v._data(e,"fxshow")||v._data(e,"fxshow",{}),"hidden"in u&&(g=u.hidden),a&&(u.hidden=!g),g?v(e).show():h.done(function(){v(e).hide()}),h.done(function(){var t;v.removeData(e,"fxshow",!0);for(t in d)v.style(e,t,d[t])});for(r=0;r<o;r++)i=m[r],f=h.createTween(i,g?u[i]:0),d[i]=u[i]||v.style(e,i),i in u||(u[i]=f.start,g&&(f.end=f.start,f.start=i==="width"||i==="height"?1:0))}}function Yn(e,t,n,r,i){return new Yn.prototype.init(e,t,n,r,i)}function Zn(e,t){var n,r={height:e},i=0;t=t?1:0;for(;i<4;i+=2-t)n=$t[i],r["margin"+n]=r["padding"+n]=e;return t&&(r.opacity=r.width=e),r}function tr(e){return v.isWindow(e)?e:e.nodeType===9?e.defaultView||e.parentWindow:!1}var n,r,i=e.document,s=e.location,o=e.navigator,u=e.jQuery,a=e.$,f=Array.prototype.push,l=Array.prototype.slice,c=Array.prototype.indexOf,h=Object.prototype.toString,p=Object.prototype.hasOwnProperty,d=String.prototype.trim,v=function(e,t){return new v.fn.init(e,t,n)},m=/[\-+]?(?:\d*\.|)\d+(?:[eE][\-+]?\d+|)/.source,g=/\S/,y=/\s+/,b=/^[\s\uFEFF\xA0]+|[\s\uFEFF\xA0]+$/g,w=/^(?:[^#<]*(<[\w\W]+>)[^>]*$|#([\w\-]*)$)/,E=/^<(\w+)\s*\/?>(?:<\/\1>|)$/,S=/^[\],:{}\s]*$/,x=/(?:^|:|,)(?:\s*\[)+/g,T=/\\(?:["\\\/bfnrt]|u[\da-fA-F]{4})/g,N=/"[^"\\\r\n]*"|true|false|null|-?(?:\d\d*\.|)\d+(?:[eE][\-+]?\d+|)/g,C=/^-ms-/,k=/-([\da-z])/gi,L=function(e,t){return(t+"").toUpperCase()},A=function(){i.addEventListener?(i.removeEventListener("DOMContentLoaded",A,!1),v.ready()):i.readyState==="complete"&&(i.detachEvent("onreadystatechange",A),v.ready())},O={};v.fn=v.prototype={constructor:v,init:function(e,n,r){var s,o,u,a;if(!e)return this;if(e.nodeType)return this.context=this[0]=e,this.length=1,this;if(typeof e=="string"){e.charAt(0)==="<"&&e.charAt(e.length-1)===">"&&e.length>=3?s=[null,e,null]:s=w.exec(e);if(s&&(s[1]||!n)){if(s[1])return n=n instanceof v?n[0]:n,a=n&&n.nodeType?n.ownerDocument||n:i,e=v.parseHTML(s[1],a,!0),E.test(s[1])&&v.isPlainObject(n)&&this.attr.call(e,n,!0),v.merge(this,e);o=i.getElementById(s[2]);if(o&&o.parentNode){if(o.id!==s[2])return r.find(e);this.length=1,this[0]=o}return this.context=i,this.selector=e,this}return!n||n.jquery?(n||r).find(e):this.constructor(n).find(e)}return v.isFunction(e)?r.ready(e):(e.selector!==t&&(this.selector=e.selector,this.context=e.context),v.makeArray(e,this))},selector:"",jquery:"1.8.3",length:0,size:function(){return this.length},toArray:function(){return l.call(this)},get:function(e){return e==null?this.toArray():e<0?this[this.length+e]:this[e]},pushStack:function(e,t,n){var r=v.merge(this.constructor(),e);return r.prevObject=this,r.context=this.context,t==="find"?r.selector=this.selector+(this.selector?" ":"")+n:t&&(r.selector=this.selector+"."+t+"("+n+")"),r},each:function(e,t){return v.each(this,e,t)},ready:function(e){return v.ready.promise().done(e),this},eq:function(e){return e=+e,e===-1?this.slice(e):this.slice(e,e+1)},first:function(){return this.eq(0)},last:function(){return this.eq(-1)},slice:function(){return this.pushStack(l.apply(this,arguments),"slice",l.call(arguments).join(","))},map:function(e){return this.pushStack(v.map(this,function(t,n){return e.call(t,n,t)}))},end:function(){return this.prevObject||this.constructor(null)},push:f,sort:[].sort,splice:[].splice},v.fn.init.prototype=v.fn,v.extend=v.fn.extend=function(){var e,n,r,i,s,o,u=arguments[0]||{},a=1,f=arguments.length,l=!1;typeof u=="boolean"&&(l=u,u=arguments[1]||{},a=2),typeof u!="object"&&!v.isFunction(u)&&(u={}),f===a&&(u=this,--a);for(;a<f;a++)if((e=arguments[a])!=null)for(n in e){r=u[n],i=e[n];if(u===i)continue;l&&i&&(v.isPlainObject(i)||(s=v.isArray(i)))?(s?(s=!1,o=r&&v.isArray(r)?r:[]):o=r&&v.isPlainObject(r)?r:{},u[n]=v.extend(l,o,i)):i!==t&&(u[n]=i)}return u},v.extend({noConflict:function(t){return e.$===v&&(e.$=a),t&&e.jQuery===v&&(e.jQuery=u),v},isReady:!1,readyWait:1,holdReady:function(e){e?v.readyWait++:v.ready(!0)},ready:function(e){if(e===!0?--v.readyWait:v.isReady)return;if(!i.body)return setTimeout(v.ready,1);v.isReady=!0;if(e!==!0&&--v.readyWait>0)return;r.resolveWith(i,[v]),v.fn.trigger&&v(i).trigger("ready").off("ready")},isFunction:function(e){return v.type(e)==="function"},isArray:Array.isArray||function(e){return v.type(e)==="array"},isWindow:function(e){return e!=null&&e==e.window},isNumeric:function(e){return!isNaN(parseFloat(e))&&isFinite(e)},type:function(e){return e==null?String(e):O[h.call(e)]||"object"},isPlainObject:function(e){if(!e||v.type(e)!=="object"||e.nodeType||v.isWindow(e))return!1;try{if(e.constructor&&!p.call(e,"constructor")&&!p.call(e.constructor.prototype,"isPrototypeOf"))return!1}catch(n){return!1}var r;for(r in e);return r===t||p.call(e,r)},isEmptyObject:function(e){var t;for(t in e)return!1;return!0},error:function(e){throw new Error(e)},parseHTML:function(e,t,n){var r;return!e||typeof e!="string"?null:(typeof t=="boolean"&&(n=t,t=0),t=t||i,(r=E.exec(e))?[t.createElement(r[1])]:(r=v.buildFragment([e],t,n?null:[]),v.merge([],(r.cacheable?v.clone(r.fragment):r.fragment).childNodes)))},parseJSON:function(t){if(!t||typeof t!="string")return null;t=v.trim(t);if(e.JSON&&e.JSON.parse)return e.JSON.parse(t);if(S.test(t.replace(T,"@").replace(N,"]").replace(x,"")))return(new Function("return "+t))();v.error("Invalid JSON: "+t)},parseXML:function(n){var r,i;if(!n||typeof n!="string")return null;try{e.DOMParser?(i=new DOMParser,r=i.parseFromString(n,"text/xml")):(r=new ActiveXObject("Microsoft.XMLDOM"),r.async="false",r.loadXML(n))}catch(s){r=t}return(!r||!r.documentElement||r.getElementsByTagName("parsererror").length)&&v.error("Invalid XML: "+n),r},noop:function(){},globalEval:function(t){t&&g.test(t)&&(e.execScript||function(t){e.eval.call(e,t)})(t)},camelCase:function(e){return e.replace(C,"ms-").replace(k,L)},nodeName:function(e,t){return e.nodeName&&e.nodeName.toLowerCase()===t.toLowerCase()},each:function(e,n,r){var i,s=0,o=e.length,u=o===t||v.isFunction(e);if(r){if(u){for(i in e)if(n.apply(e[i],r)===!1)break}else for(;s<o;)if(n.apply(e[s++],r)===!1)break}else if(u){for(i in e)if(n.call(e[i],i,e[i])===!1)break}else for(;s<o;)if(n.call(e[s],s,e[s++])===!1)break;return e},trim:d&&!d.call("\ufeff\u00a0")?function(e){return e==null?"":d.call(e)}:function(e){return e==null?"":(e+"").replace(b,"")},makeArray:function(e,t){var n,r=t||[];return e!=null&&(n=v.type(e),e.length==null||n==="string"||n==="function"||n==="regexp"||v.isWindow(e)?f.call(r,e):v.merge(r,e)),r},inArray:function(e,t,n){var r;if(t){if(c)return c.call(t,e,n);r=t.length,n=n?n<0?Math.max(0,r+n):n:0;for(;n<r;n++)if(n in t&&t[n]===e)return n}return-1},merge:function(e,n){var r=n.length,i=e.length,s=0;if(typeof r=="number")for(;s<r;s++)e[i++]=n[s];else while(n[s]!==t)e[i++]=n[s++];return e.length=i,e},grep:function(e,t,n){var r,i=[],s=0,o=e.length;n=!!n;for(;s<o;s++)r=!!t(e[s],s),n!==r&&i.push(e[s]);return i},map:function(e,n,r){var i,s,o=[],u=0,a=e.length,f=e instanceof v||a!==t&&typeof a=="number"&&(a>0&&e[0]&&e[a-1]||a===0||v.isArray(e));if(f)for(;u<a;u++)i=n(e[u],u,r),i!=null&&(o[o.length]=i);else for(s in e)i=n(e[s],s,r),i!=null&&(o[o.length]=i);return o.concat.apply([],o)},guid:1,proxy:function(e,n){var r,i,s;return typeof n=="string"&&(r=e[n],n=e,e=r),v.isFunction(e)?(i=l.call(arguments,2),s=function(){return e.apply(n,i.concat(l.call(arguments)))},s.guid=e.guid=e.guid||v.guid++,s):t},access:function(e,n,r,i,s,o,u){var a,f=r==null,l=0,c=e.length;if(r&&typeof r=="object"){for(l in r)v.access(e,n,l,r[l],1,o,i);s=1}else if(i!==t){a=u===t&&v.isFunction(i),f&&(a?(a=n,n=function(e,t,n){return a.call(v(e),n)}):(n.call(e,i),n=null));if(n)for(;l<c;l++)n(e[l],r,a?i.call(e[l],l,n(e[l],r)):i,u);s=1}return s?e:f?n.call(e):c?n(e[0],r):o},now:function(){return(new Date).getTime()}}),v.ready.promise=function(t){if(!r){r=v.Deferred();if(i.readyState==="complete")setTimeout(v.ready,1);else if(i.addEventListener)i.addEventListener("DOMContentLoaded",A,!1),e.addEventListener("load",v.ready,!1);else{i.attachEvent("onreadystatechange",A),e.attachEvent("onload",v.ready);var n=!1;try{n=e.frameElement==null&&i.documentElement}catch(s){}n&&n.doScroll&&function o(){if(!v.isReady){try{n.doScroll("left")}catch(e){return setTimeout(o,50)}v.ready()}}()}}return r.promise(t)},v.each("Boolean Number String Function Array Date RegExp Object".split(" "),function(e,t){O["[object "+t+"]"]=t.toLowerCase()}),n=v(i);var M={};v.Callbacks=function(e){e=typeof e=="string"?M[e]||_(e):v.extend({},e);var n,r,i,s,o,u,a=[],f=!e.once&&[],l=function(t){n=e.memory&&t,r=!0,u=s||0,s=0,o=a.length,i=!0;for(;a&&u<o;u++)if(a[u].apply(t[0],t[1])===!1&&e.stopOnFalse){n=!1;break}i=!1,a&&(f?f.length&&l(f.shift()):n?a=[]:c.disable())},c={add:function(){if(a){var t=a.length;(function r(t){v.each(t,function(t,n){var i=v.type(n);i==="function"?(!e.unique||!c.has(n))&&a.push(n):n&&n.length&&i!=="string"&&r(n)})})(arguments),i?o=a.length:n&&(s=t,l(n))}return this},remove:function(){return a&&v.each(arguments,function(e,t){var n;while((n=v.inArray(t,a,n))>-1)a.splice(n,1),i&&(n<=o&&o--,n<=u&&u--)}),this},has:function(e){return v.inArray(e,a)>-1},empty:function(){return a=[],this},disable:function(){return a=f=n=t,this},disabled:function(){return!a},lock:function(){return f=t,n||c.disable(),this},locked:function(){return!f},fireWith:function(e,t){return t=t||[],t=[e,t.slice?t.slice():t],a&&(!r||f)&&(i?f.push(t):l(t)),this},fire:function(){return c.fireWith(this,arguments),this},fired:function(){return!!r}};return c},v.extend({Deferred:function(e){var t=[["resolve","done",v.Callbacks("once memory"),"resolved"],["reject","fail",v.Callbacks("once memory"),"rejected"],["notify","progress",v.Callbacks("memory")]],n="pending",r={state:function(){return n},always:function(){return i.done(arguments).fail(arguments),this},then:function(){var e=arguments;return v.Deferred(function(n){v.each(t,function(t,r){var s=r[0],o=e[t];i[r[1]](v.isFunction(o)?function(){var e=o.apply(this,arguments);e&&v.isFunction(e.promise)?e.promise().done(n.resolve).fail(n.reject).progress(n.notify):n[s+"With"](this===i?n:this,[e])}:n[s])}),e=null}).promise()},promise:function(e){return e!=null?v.extend(e,r):r}},i={};return r.pipe=r.then,v.each(t,function(e,s){var o=s[2],u=s[3];r[s[1]]=o.add,u&&o.add(function(){n=u},t[e^1][2].disable,t[2][2].lock),i[s[0]]=o.fire,i[s[0]+"With"]=o.fireWith}),r.promise(i),e&&e.call(i,i),i},when:function(e){var t=0,n=l.call(arguments),r=n.length,i=r!==1||e&&v.isFunction(e.promise)?r:0,s=i===1?e:v.Deferred(),o=function(e,t,n){return function(r){t[e]=this,n[e]=arguments.length>1?l.call(arguments):r,n===u?s.notifyWith(t,n):--i||s.resolveWith(t,n)}},u,a,f;if(r>1){u=new Array(r),a=new Array(r),f=new Array(r);for(;t<r;t++)n[t]&&v.isFunction(n[t].promise)?n[t].promise().done(o(t,f,n)).fail(s.reject).progress(o(t,a,u)):--i}return i||s.resolveWith(f,n),s.promise()}}),v.support=function(){var t,n,r,s,o,u,a,f,l,c,h,p=i.createElement("div");p.setAttribute("className","t"),p.innerHTML="  <link/><table></table><a href='/a'>a</a><input type='checkbox'/>",n=p.getElementsByTagName("*"),r=p.getElementsByTagName("a")[0];if(!n||!r||!n.length)return{};s=i.createElement("select"),o=s.appendChild(i.createElement("option")),u=p.getElementsByTagName("input")[0],r.style.cssText="top:1px;float:left;opacity:.5",t={leadingWhitespace:p.firstChild.nodeType===3,tbody:!p.getElementsByTagName("tbody").length,htmlSerialize:!!p.getElementsByTagName("link").length,style:/top/.test(r.getAttribute("style")),hrefNormalized:r.getAttribute("href")==="/a",opacity:/^0.5/.test(r.style.opacity),cssFloat:!!r.style.cssFloat,checkOn:u.value==="on",optSelected:o.selected,getSetAttribute:p.className!=="t",enctype:!!i.createElement("form").enctype,html5Clone:i.createElement("nav").cloneNode(!0).outerHTML!=="<:nav></:nav>",boxModel:i.compatMode==="CSS1Compat",submitBubbles:!0,changeBubbles:!0,focusinBubbles:!1,deleteExpando:!0,noCloneEvent:!0,inlineBlockNeedsLayout:!1,shrinkWrapBlocks:!1,reliableMarginRight:!0,boxSizingReliable:!0,pixelPosition:!1},u.checked=!0,t.noCloneChecked=u.cloneNode(!0).checked,s.disabled=!0,t.optDisabled=!o.disabled;try{delete p.test}catch(d){t.deleteExpando=!1}!p.addEventListener&&p.attachEvent&&p.fireEvent&&(p.attachEvent("onclick",h=function(){t.noCloneEvent=!1}),p.cloneNode(!0).fireEvent("onclick"),p.detachEvent("onclick",h)),u=i.createElement("input"),u.value="t",u.setAttribute("type","radio"),t.radioValue=u.value==="t",u.setAttribute("checked","checked"),u.setAttribute("name","t"),p.appendChild(u),a=i.createDocumentFragment(),a.appendChild(p.lastChild),t.checkClone=a.cloneNode(!0).cloneNode(!0).lastChild.checked,t.appendChecked=u.checked,a.removeChild(u),a.appendChild(p);if(p.attachEvent)for(l in{submit:!0,change:!0,focusin:!0})f="on"+l,c=f in p,c||(p.setAttribute(f,"return;"),c=typeof p[f]=="function"),t[l+"Bubbles"]=c;return v(function(){var n,r,s,o,u="padding:0;margin:0;border:0;display:block;overflow:hidden;",a=i.getElementsByTagName("body")[0];if(!a)return;n=i.createElement("div"),n.style.cssText="visibility:hidden;border:0;width:0;height:0;position:static;top:0;margin-top:1px",a.insertBefore(n,a.firstChild),r=i.createElement("div"),n.appendChild(r),r.innerHTML="<table><tr><td></td><td>t</td></tr></table>",s=r.getElementsByTagName("td"),s[0].style.cssText="padding:0;margin:0;border:0;display:none",c=s[0].offsetHeight===0,s[0].style.display="",s[1].style.display="none",t.reliableHiddenOffsets=c&&s[0].offsetHeight===0,r.innerHTML="",r.style.cssText="box-sizing:border-box;-moz-box-sizing:border-box;-webkit-box-sizing:border-box;padding:1px;border:1px;display:block;width:4px;margin-top:1%;position:absolute;top:1%;",t.boxSizing=r.offsetWidth===4,t.doesNotIncludeMarginInBodyOffset=a.offsetTop!==1,e.getComputedStyle&&(t.pixelPosition=(e.getComputedStyle(r,null)||{}).top!=="1%",t.boxSizingReliable=(e.getComputedStyle(r,null)||{width:"4px"}).width==="4px",o=i.createElement("div"),o.style.cssText=r.style.cssText=u,o.style.marginRight=o.style.width="0",r.style.width="1px",r.appendChild(o),t.reliableMarginRight=!parseFloat((e.getComputedStyle(o,null)||{}).marginRight)),typeof r.style.zoom!="undefined"&&(r.innerHTML="",r.style.cssText=u+"width:1px;padding:1px;display:inline;zoom:1",t.inlineBlockNeedsLayout=r.offsetWidth===3,r.style.display="block",r.style.overflow="visible",r.innerHTML="<div></div>",r.firstChild.style.width="5px",t.shrinkWrapBlocks=r.offsetWidth!==3,n.style.zoom=1),a.removeChild(n),n=r=s=o=null}),a.removeChild(p),n=r=s=o=u=a=p=null,t}();var D=/(?:\{[\s\S]*\}|\[[\s\S]*\])$/,P=/([A-Z])/g;v.extend({cache:{},deletedIds:[],uuid:0,expando:"jQuery"+(v.fn.jquery+Math.random()).replace(/\D/g,""),noData:{embed:!0,object:"clsid:D27CDB6E-AE6D-11cf-96B8-444553540000",applet:!0},hasData:function(e){return e=e.nodeType?v.cache[e[v.expando]]:e[v.expando],!!e&&!B(e)},data:function(e,n,r,i){if(!v.acceptData(e))return;var s,o,u=v.expando,a=typeof n=="string",f=e.nodeType,l=f?v.cache:e,c=f?e[u]:e[u]&&u;if((!c||!l[c]||!i&&!l[c].data)&&a&&r===t)return;c||(f?e[u]=c=v.deletedIds.pop()||v.guid++:c=u),l[c]||(l[c]={},f||(l[c].toJSON=v.noop));if(typeof n=="object"||typeof n=="function")i?l[c]=v.extend(l[c],n):l[c].data=v.extend(l[c].data,n);return s=l[c],i||(s.data||(s.data={}),s=s.data),r!==t&&(s[v.camelCase(n)]=r),a?(o=s[n],o==null&&(o=s[v.camelCase(n)])):o=s,o},removeData:function(e,t,n){if(!v.acceptData(e))return;var r,i,s,o=e.nodeType,u=o?v.cache:e,a=o?e[v.expando]:v.expando;if(!u[a])return;if(t){r=n?u[a]:u[a].data;if(r){v.isArray(t)||(t in r?t=[t]:(t=v.camelCase(t),t in r?t=[t]:t=t.split(" ")));for(i=0,s=t.length;i<s;i++)delete r[t[i]];if(!(n?B:v.isEmptyObject)(r))return}}if(!n){delete u[a].data;if(!B(u[a]))return}o?v.cleanData([e],!0):v.support.deleteExpando||u!=u.window?delete u[a]:u[a]=null},_data:function(e,t,n){return v.data(e,t,n,!0)},acceptData:function(e){var t=e.nodeName&&v.noData[e.nodeName.toLowerCase()];return!t||t!==!0&&e.getAttribute("classid")===t}}),v.fn.extend({data:function(e,n){var r,i,s,o,u,a=this[0],f=0,l=null;if(e===t){if(this.length){l=v.data(a);if(a.nodeType===1&&!v._data(a,"parsedAttrs")){s=a.attributes;for(u=s.length;f<u;f++)o=s[f].name,o.indexOf("data-")||(o=v.camelCase(o.substring(5)),H(a,o,l[o]));v._data(a,"parsedAttrs",!0)}}return l}return typeof e=="object"?this.each(function(){v.data(this,e)}):(r=e.split(".",2),r[1]=r[1]?"."+r[1]:"",i=r[1]+"!",v.access(this,function(n){if(n===t)return l=this.triggerHandler("getData"+i,[r[0]]),l===t&&a&&(l=v.data(a,e),l=H(a,e,l)),l===t&&r[1]?this.data(r[0]):l;r[1]=n,this.each(function(){var t=v(this);t.triggerHandler("setData"+i,r),v.data(this,e,n),t.triggerHandler("changeData"+i,r)})},null,n,arguments.length>1,null,!1))},removeData:function(e){return this.each(function(){v.removeData(this,e)})}}),v.extend({queue:function(e,t,n){var r;if(e)return t=(t||"fx")+"queue",r=v._data(e,t),n&&(!r||v.isArray(n)?r=v._data(e,t,v.makeArray(n)):r.push(n)),r||[]},dequeue:function(e,t){t=t||"fx";var n=v.queue(e,t),r=n.length,i=n.shift(),s=v._queueHooks(e,t),o=function(){v.dequeue(e,t)};i==="inprogress"&&(i=n.shift(),r--),i&&(t==="fx"&&n.unshift("inprogress"),delete s.stop,i.call(e,o,s)),!r&&s&&s.empty.fire()},_queueHooks:function(e,t){var n=t+"queueHooks";return v._data(e,n)||v._data(e,n,{empty:v.Callbacks("once memory").add(function(){v.removeData(e,t+"queue",!0),v.removeData(e,n,!0)})})}}),v.fn.extend({queue:function(e,n){var r=2;return typeof e!="string"&&(n=e,e="fx",r--),arguments.length<r?v.queue(this[0],e):n===t?this:this.each(function(){var t=v.queue(this,e,n);v._queueHooks(this,e),e==="fx"&&t[0]!=="inprogress"&&v.dequeue(this,e)})},dequeue:function(e){return this.each(function(){v.dequeue(this,e)})},delay:function(e,t){return e=v.fx?v.fx.speeds[e]||e:e,t=t||"fx",this.queue(t,function(t,n){var r=setTimeout(t,e);n.stop=function(){clearTimeout(r)}})},clearQueue:function(e){return this.queue(e||"fx",[])},promise:function(e,n){var r,i=1,s=v.Deferred(),o=this,u=this.length,a=function(){--i||s.resolveWith(o,[o])};typeof e!="string"&&(n=e,e=t),e=e||"fx";while(u--)r=v._data(o[u],e+"queueHooks"),r&&r.empty&&(i++,r.empty.add(a));return a(),s.promise(n)}});var j,F,I,q=/[\t\r\n]/g,R=/\r/g,U=/^(?:button|input)$/i,z=/^(?:button|input|object|select|textarea)$/i,W=/^a(?:rea|)$/i,X=/^(?:autofocus|autoplay|async|checked|controls|defer|disabled|hidden|loop|multiple|open|readonly|required|scoped|selected)$/i,V=v.support.getSetAttribute;v.fn.extend({attr:function(e,t){return v.access(this,v.attr,e,t,arguments.length>1)},removeAttr:function(e){return this.each(function(){v.removeAttr(this,e)})},prop:function(e,t){return v.access(this,v.prop,e,t,arguments.length>1)},removeProp:function(e){return e=v.propFix[e]||e,this.each(function(){try{this[e]=t,delete this[e]}catch(n){}})},addClass:function(e){var t,n,r,i,s,o,u;if(v.isFunction(e))return this.each(function(t){v(this).addClass(e.call(this,t,this.className))});if(e&&typeof e=="string"){t=e.split(y);for(n=0,r=this.length;n<r;n++){i=this[n];if(i.nodeType===1)if(!i.className&&t.length===1)i.className=e;else{s=" "+i.className+" ";for(o=0,u=t.length;o<u;o++)s.indexOf(" "+t[o]+" ")<0&&(s+=t[o]+" ");i.className=v.trim(s)}}}return this},removeClass:function(e){var n,r,i,s,o,u,a;if(v.isFunction(e))return this.each(function(t){v(this).removeClass(e.call(this,t,this.className))});if(e&&typeof e=="string"||e===t){n=(e||"").split(y);for(u=0,a=this.length;u<a;u++){i=this[u];if(i.nodeType===1&&i.className){r=(" "+i.className+" ").replace(q," ");for(s=0,o=n.length;s<o;s++)while(r.indexOf(" "+n[s]+" ")>=0)r=r.replace(" "+n[s]+" "," ");i.className=e?v.trim(r):""}}}return this},toggleClass:function(e,t){var n=typeof e,r=typeof t=="boolean";return v.isFunction(e)?this.each(function(n){v(this).toggleClass(e.call(this,n,this.className,t),t)}):this.each(function(){if(n==="string"){var i,s=0,o=v(this),u=t,a=e.split(y);while(i=a[s++])u=r?u:!o.hasClass(i),o[u?"addClass":"removeClass"](i)}else if(n==="undefined"||n==="boolean")this.className&&v._data(this,"__className__",this.className),this.className=this.className||e===!1?"":v._data(this,"__className__")||""})},hasClass:function(e){var t=" "+e+" ",n=0,r=this.length;for(;n<r;n++)if(this[n].nodeType===1&&(" "+this[n].className+" ").replace(q," ").indexOf(t)>=0)return!0;return!1},val:function(e){var n,r,i,s=this[0];if(!arguments.length){if(s)return n=v.valHooks[s.type]||v.valHooks[s.nodeName.toLowerCase()],n&&"get"in n&&(r=n.get(s,"value"))!==t?r:(r=s.value,typeof r=="string"?r.replace(R,""):r==null?"":r);return}return i=v.isFunction(e),this.each(function(r){var s,o=v(this);if(this.nodeType!==1)return;i?s=e.call(this,r,o.val()):s=e,s==null?s="":typeof s=="number"?s+="":v.isArray(s)&&(s=v.map(s,function(e){return e==null?"":e+""})),n=v.valHooks[this.type]||v.valHooks[this.nodeName.toLowerCase()];if(!n||!("set"in n)||n.set(this,s,"value")===t)this.value=s})}}),v.extend({valHooks:{option:{get:function(e){var t=e.attributes.value;return!t||t.specified?e.value:e.text}},select:{get:function(e){var t,n,r=e.options,i=e.selectedIndex,s=e.type==="select-one"||i<0,o=s?null:[],u=s?i+1:r.length,a=i<0?u:s?i:0;for(;a<u;a++){n=r[a];if((n.selected||a===i)&&(v.support.optDisabled?!n.disabled:n.getAttribute("disabled")===null)&&(!n.parentNode.disabled||!v.nodeName(n.parentNode,"optgroup"))){t=v(n).val();if(s)return t;o.push(t)}}return o},set:function(e,t){var n=v.makeArray(t);return v(e).find("option").each(function(){this.selected=v.inArray(v(this).val(),n)>=0}),n.length||(e.selectedIndex=-1),n}}},attrFn:{},attr:function(e,n,r,i){var s,o,u,a=e.nodeType;if(!e||a===3||a===8||a===2)return;if(i&&v.isFunction(v.fn[n]))return v(e)[n](r);if(typeof e.getAttribute=="undefined")return v.prop(e,n,r);u=a!==1||!v.isXMLDoc(e),u&&(n=n.toLowerCase(),o=v.attrHooks[n]||(X.test(n)?F:j));if(r!==t){if(r===null){v.removeAttr(e,n);return}return o&&"set"in o&&u&&(s=o.set(e,r,n))!==t?s:(e.setAttribute(n,r+""),r)}return o&&"get"in o&&u&&(s=o.get(e,n))!==null?s:(s=e.getAttribute(n),s===null?t:s)},removeAttr:function(e,t){var n,r,i,s,o=0;if(t&&e.nodeType===1){r=t.split(y);for(;o<r.length;o++)i=r[o],i&&(n=v.propFix[i]||i,s=X.test(i),s||v.attr(e,i,""),e.removeAttribute(V?i:n),s&&n in e&&(e[n]=!1))}},attrHooks:{type:{set:function(e,t){if(U.test(e.nodeName)&&e.parentNode)v.error("type property can't be changed");else if(!v.support.radioValue&&t==="radio"&&v.nodeName(e,"input")){var n=e.value;return e.setAttribute("type",t),n&&(e.value=n),t}}},value:{get:function(e,t){return j&&v.nodeName(e,"button")?j.get(e,t):t in e?e.value:null},set:function(e,t,n){if(j&&v.nodeName(e,"button"))return j.set(e,t,n);e.value=t}}},propFix:{tabindex:"tabIndex",readonly:"readOnly","for":"htmlFor","class":"className",maxlength:"maxLength",cellspacing:"cellSpacing",cellpadding:"cellPadding",rowspan:"rowSpan",colspan:"colSpan",usemap:"useMap",frameborder:"frameBorder",contenteditable:"contentEditable"},prop:function(e,n,r){var i,s,o,u=e.nodeType;if(!e||u===3||u===8||u===2)return;return o=u!==1||!v.isXMLDoc(e),o&&(n=v.propFix[n]||n,s=v.propHooks[n]),r!==t?s&&"set"in s&&(i=s.set(e,r,n))!==t?i:e[n]=r:s&&"get"in s&&(i=s.get(e,n))!==null?i:e[n]},propHooks:{tabIndex:{get:function(e){var n=e.getAttributeNode("tabindex");return n&&n.specified?parseInt(n.value,10):z.test(e.nodeName)||W.test(e.nodeName)&&e.href?0:t}}}}),F={get:function(e,n){var r,i=v.prop(e,n);return i===!0||typeof i!="boolean"&&(r=e.getAttributeNode(n))&&r.nodeValue!==!1?n.toLowerCase():t},set:function(e,t,n){var r;return t===!1?v.removeAttr(e,n):(r=v.propFix[n]||n,r in e&&(e[r]=!0),e.setAttribute(n,n.toLowerCase())),n}},V||(I={name:!0,id:!0,coords:!0},j=v.valHooks.button={get:function(e,n){var r;return r=e.getAttributeNode(n),r&&(I[n]?r.value!=="":r.specified)?r.value:t},set:function(e,t,n){var r=e.getAttributeNode(n);return r||(r=i.createAttribute(n),e.setAttributeNode(r)),r.value=t+""}},v.each(["width","height"],function(e,t){v.attrHooks[t]=v.extend(v.attrHooks[t],{set:function(e,n){if(n==="")return e.setAttribute(t,"auto"),n}})}),v.attrHooks.contenteditable={get:j.get,set:function(e,t,n){t===""&&(t="false"),j.set(e,t,n)}}),v.support.hrefNormalized||v.each(["href","src","width","height"],function(e,n){v.attrHooks[n]=v.extend(v.attrHooks[n],{get:function(e){var r=e.getAttribute(n,2);return r===null?t:r}})}),v.support.style||(v.attrHooks.style={get:function(e){return e.style.cssText.toLowerCase()||t},set:function(e,t){return e.style.cssText=t+""}}),v.support.optSelected||(v.propHooks.selected=v.extend(v.propHooks.selected,{get:function(e){var t=e.parentNode;return t&&(t.selectedIndex,t.parentNode&&t.parentNode.selectedIndex),null}})),v.support.enctype||(v.propFix.enctype="encoding"),v.support.checkOn||v.each(["radio","checkbox"],function(){v.valHooks[this]={get:function(e){return e.getAttribute("value")===null?"on":e.value}}}),v.each(["radio","checkbox"],function(){v.valHooks[this]=v.extend(v.valHooks[this],{set:function(e,t){if(v.isArray(t))return e.checked=v.inArray(v(e).val(),t)>=0}})});var $=/^(?:textarea|input|select)$/i,J=/^([^\.]*|)(?:\.(.+)|)$/,K=/(?:^|\s)hover(\.\S+|)\b/,Q=/^key/,G=/^(?:mouse|contextmenu)|click/,Y=/^(?:focusinfocus|focusoutblur)$/,Z=function(e){return v.event.special.hover?e:e.replace(K,"mouseenter$1 mouseleave$1")};v.event={add:function(e,n,r,i,s){var o,u,a,f,l,c,h,p,d,m,g;if(e.nodeType===3||e.nodeType===8||!n||!r||!(o=v._data(e)))return;r.handler&&(d=r,r=d.handler,s=d.selector),r.guid||(r.guid=v.guid++),a=o.events,a||(o.events=a={}),u=o.handle,u||(o.handle=u=function(e){return typeof v=="undefined"||!!e&&v.event.triggered===e.type?t:v.event.dispatch.apply(u.elem,arguments)},u.elem=e),n=v.trim(Z(n)).split(" ");for(f=0;f<n.length;f++){l=J.exec(n[f])||[],c=l[1],h=(l[2]||"").split(".").sort(),g=v.event.special[c]||{},c=(s?g.delegateType:g.bindType)||c,g=v.event.special[c]||{},p=v.extend({type:c,origType:l[1],data:i,handler:r,guid:r.guid,selector:s,needsContext:s&&v.expr.match.needsContext.test(s),namespace:h.join(".")},d),m=a[c];if(!m){m=a[c]=[],m.delegateCount=0;if(!g.setup||g.setup.call(e,i,h,u)===!1)e.addEventListener?e.addEventListener(c,u,!1):e.attachEvent&&e.attachEvent("on"+c,u)}g.add&&(g.add.call(e,p),p.handler.guid||(p.handler.guid=r.guid)),s?m.splice(m.delegateCount++,0,p):m.push(p),v.event.global[c]=!0}e=null},global:{},remove:function(e,t,n,r,i){var s,o,u,a,f,l,c,h,p,d,m,g=v.hasData(e)&&v._data(e);if(!g||!(h=g.events))return;t=v.trim(Z(t||"")).split(" ");for(s=0;s<t.length;s++){o=J.exec(t[s])||[],u=a=o[1],f=o[2];if(!u){for(u in h)v.event.remove(e,u+t[s],n,r,!0);continue}p=v.event.special[u]||{},u=(r?p.delegateType:p.bindType)||u,d=h[u]||[],l=d.length,f=f?new RegExp("(^|\\.)"+f.split(".").sort().join("\\.(?:.*\\.|)")+"(\\.|$)"):null;for(c=0;c<d.length;c++)m=d[c],(i||a===m.origType)&&(!n||n.guid===m.guid)&&(!f||f.test(m.namespace))&&(!r||r===m.selector||r==="**"&&m.selector)&&(d.splice(c--,1),m.selector&&d.delegateCount--,p.remove&&p.remove.call(e,m));d.length===0&&l!==d.length&&((!p.teardown||p.teardown.call(e,f,g.handle)===!1)&&v.removeEvent(e,u,g.handle),delete h[u])}v.isEmptyObject(h)&&(delete g.handle,v.removeData(e,"events",!0))},customEvent:{getData:!0,setData:!0,changeData:!0},trigger:function(n,r,s,o){if(!s||s.nodeType!==3&&s.nodeType!==8){var u,a,f,l,c,h,p,d,m,g,y=n.type||n,b=[];if(Y.test(y+v.event.triggered))return;y.indexOf("!")>=0&&(y=y.slice(0,-1),a=!0),y.indexOf(".")>=0&&(b=y.split("."),y=b.shift(),b.sort());if((!s||v.event.customEvent[y])&&!v.event.global[y])return;n=typeof n=="object"?n[v.expando]?n:new v.Event(y,n):new v.Event(y),n.type=y,n.isTrigger=!0,n.exclusive=a,n.namespace=b.join("."),n.namespace_re=n.namespace?new RegExp("(^|\\.)"+b.join("\\.(?:.*\\.|)")+"(\\.|$)"):null,h=y.indexOf(":")<0?"on"+y:"";if(!s){u=v.cache;for(f in u)u[f].events&&u[f].events[y]&&v.event.trigger(n,r,u[f].handle.elem,!0);return}n.result=t,n.target||(n.target=s),r=r!=null?v.makeArray(r):[],r.unshift(n),p=v.event.special[y]||{};if(p.trigger&&p.trigger.apply(s,r)===!1)return;m=[[s,p.bindType||y]];if(!o&&!p.noBubble&&!v.isWindow(s)){g=p.delegateType||y,l=Y.test(g+y)?s:s.parentNode;for(c=s;l;l=l.parentNode)m.push([l,g]),c=l;c===(s.ownerDocument||i)&&m.push([c.defaultView||c.parentWindow||e,g])}for(f=0;f<m.length&&!n.isPropagationStopped();f++)l=m[f][0],n.type=m[f][1],d=(v._data(l,"events")||{})[n.type]&&v._data(l,"handle"),d&&d.apply(l,r),d=h&&l[h],d&&v.acceptData(l)&&d.apply&&d.apply(l,r)===!1&&n.preventDefault();return n.type=y,!o&&!n.isDefaultPrevented()&&(!p._default||p._default.apply(s.ownerDocument,r)===!1)&&(y!=="click"||!v.nodeName(s,"a"))&&v.acceptData(s)&&h&&s[y]&&(y!=="focus"&&y!=="blur"||n.target.offsetWidth!==0)&&!v.isWindow(s)&&(c=s[h],c&&(s[h]=null),v.event.triggered=y,s[y](),v.event.triggered=t,c&&(s[h]=c)),n.result}return},dispatch:function(n){n=v.event.fix(n||e.event);var r,i,s,o,u,a,f,c,h,p,d=(v._data(this,"events")||{})[n.type]||[],m=d.delegateCount,g=l.call(arguments),y=!n.exclusive&&!n.namespace,b=v.event.special[n.type]||{},w=[];g[0]=n,n.delegateTarget=this;if(b.preDispatch&&b.preDispatch.call(this,n)===!1)return;if(m&&(!n.button||n.type!=="click"))for(s=n.target;s!=this;s=s.parentNode||this)if(s.disabled!==!0||n.type!=="click"){u={},f=[];for(r=0;r<m;r++)c=d[r],h=c.selector,u[h]===t&&(u[h]=c.needsContext?v(h,this).index(s)>=0:v.find(h,this,null,[s]).length),u[h]&&f.push(c);f.length&&w.push({elem:s,matches:f})}d.length>m&&w.push({elem:this,matches:d.slice(m)});for(r=0;r<w.length&&!n.isPropagationStopped();r++){a=w[r],n.currentTarget=a.elem;for(i=0;i<a.matches.length&&!n.isImmediatePropagationStopped();i++){c=a.matches[i];if(y||!n.namespace&&!c.namespace||n.namespace_re&&n.namespace_re.test(c.namespace))n.data=c.data,n.handleObj=c,o=((v.event.special[c.origType]||{}).handle||c.handler).apply(a.elem,g),o!==t&&(n.result=o,o===!1&&(n.preventDefault(),n.stopPropagation()))}}return b.postDispatch&&b.postDispatch.call(this,n),n.result},props:"attrChange attrName relatedNode srcElement altKey bubbles cancelable ctrlKey currentTarget eventPhase metaKey relatedTarget shiftKey target timeStamp view which".split(" "),fixHooks:{},keyHooks:{props:"char charCode key keyCode".split(" "),filter:function(e,t){return e.which==null&&(e.which=t.charCode!=null?t.charCode:t.keyCode),e}},mouseHooks:{props:"button buttons clientX clientY fromElement offsetX offsetY pageX pageY screenX screenY toElement".split(" "),filter:function(e,n){var r,s,o,u=n.button,a=n.fromElement;return e.pageX==null&&n.clientX!=null&&(r=e.target.ownerDocument||i,s=r.documentElement,o=r.body,e.pageX=n.clientX+(s&&s.scrollLeft||o&&o.scrollLeft||0)-(s&&s.clientLeft||o&&o.clientLeft||0),e.pageY=n.clientY+(s&&s.scrollTop||o&&o.scrollTop||0)-(s&&s.clientTop||o&&o.clientTop||0)),!e.relatedTarget&&a&&(e.relatedTarget=a===e.target?n.toElement:a),!e.which&&u!==t&&(e.which=u&1?1:u&2?3:u&4?2:0),e}},fix:function(e){if(e[v.expando])return e;var t,n,r=e,s=v.event.fixHooks[e.type]||{},o=s.props?this.props.concat(s.props):this.props;e=v.Event(r);for(t=o.length;t;)n=o[--t],e[n]=r[n];return e.target||(e.target=r.srcElement||i),e.target.nodeType===3&&(e.target=e.target.parentNode),e.metaKey=!!e.metaKey,s.filter?s.filter(e,r):e},special:{load:{noBubble:!0},focus:{delegateType:"focusin"},blur:{delegateType:"focusout"},beforeunload:{setup:function(e,t,n){v.isWindow(this)&&(this.onbeforeunload=n)},teardown:function(e,t){this.onbeforeunload===t&&(this.onbeforeunload=null)}}},simulate:function(e,t,n,r){var i=v.extend(new v.Event,n,{type:e,isSimulated:!0,originalEvent:{}});r?v.event.trigger(i,null,t):v.event.dispatch.call(t,i),i.isDefaultPrevented()&&n.preventDefault()}},v.event.handle=v.event.dispatch,v.removeEvent=i.removeEventListener?function(e,t,n){e.removeEventListener&&e.removeEventListener(t,n,!1)}:function(e,t,n){var r="on"+t;e.detachEvent&&(typeof e[r]=="undefined"&&(e[r]=null),e.detachEvent(r,n))},v.Event=function(e,t){if(!(this instanceof v.Event))return new v.Event(e,t);e&&e.type?(this.originalEvent=e,this.type=e.type,this.isDefaultPrevented=e.defaultPrevented||e.returnValue===!1||e.getPreventDefault&&e.getPreventDefault()?tt:et):this.type=e,t&&v.extend(this,t),this.timeStamp=e&&e.timeStamp||v.now(),this[v.expando]=!0},v.Event.prototype={preventDefault:function(){this.isDefaultPrevented=tt;var e=this.originalEvent;if(!e)return;e.preventDefault?e.preventDefault():e.returnValue=!1},stopPropagation:function(){this.isPropagationStopped=tt;var e=this.originalEvent;if(!e)return;e.stopPropagation&&e.stopPropagation(),e.cancelBubble=!0},stopImmediatePropagation:function(){this.isImmediatePropagationStopped=tt,this.stopPropagation()},isDefaultPrevented:et,isPropagationStopped:et,isImmediatePropagationStopped:et},v.each({mouseenter:"mouseover",mouseleave:"mouseout"},function(e,t){v.event.special[e]={delegateType:t,bindType:t,handle:function(e){var n,r=this,i=e.relatedTarget,s=e.handleObj,o=s.selector;if(!i||i!==r&&!v.contains(r,i))e.type=s.origType,n=s.handler.apply(this,arguments),e.type=t;return n}}}),v.support.submitBubbles||(v.event.special.submit={setup:function(){if(v.nodeName(this,"form"))return!1;v.event.add(this,"click._submit keypress._submit",function(e){var n=e.target,r=v.nodeName(n,"input")||v.nodeName(n,"button")?n.form:t;r&&!v._data(r,"_submit_attached")&&(v.event.add(r,"submit._submit",function(e){e._submit_bubble=!0}),v._data(r,"_submit_attached",!0))})},postDispatch:function(e){e._submit_bubble&&(delete e._submit_bubble,this.parentNode&&!e.isTrigger&&v.event.simulate("submit",this.parentNode,e,!0))},teardown:function(){if(v.nodeName(this,"form"))return!1;v.event.remove(this,"._submit")}}),v.support.changeBubbles||(v.event.special.change={setup:function(){if($.test(this.nodeName)){if(this.type==="checkbox"||this.type==="radio")v.event.add(this,"propertychange._change",function(e){e.originalEvent.propertyName==="checked"&&(this._just_changed=!0)}),v.event.add(this,"click._change",function(e){this._just_changed&&!e.isTrigger&&(this._just_changed=!1),v.event.simulate("change",this,e,!0)});return!1}v.event.add(this,"beforeactivate._change",function(e){var t=e.target;$.test(t.nodeName)&&!v._data(t,"_change_attached")&&(v.event.add(t,"change._change",function(e){this.parentNode&&!e.isSimulated&&!e.isTrigger&&v.event.simulate("change",this.parentNode,e,!0)}),v._data(t,"_change_attached",!0))})},handle:function(e){var t=e.target;if(this!==t||e.isSimulated||e.isTrigger||t.type!=="radio"&&t.type!=="checkbox")return e.handleObj.handler.apply(this,arguments)},teardown:function(){return v.event.remove(this,"._change"),!$.test(this.nodeName)}}),v.support.focusinBubbles||v.each({focus:"focusin",blur:"focusout"},function(e,t){var n=0,r=function(e){v.event.simulate(t,e.target,v.event.fix(e),!0)};v.event.special[t]={setup:function(){n++===0&&i.addEventListener(e,r,!0)},teardown:function(){--n===0&&i.removeEventListener(e,r,!0)}}}),v.fn.extend({on:function(e,n,r,i,s){var o,u;if(typeof e=="object"){typeof n!="string"&&(r=r||n,n=t);for(u in e)this.on(u,n,r,e[u],s);return this}r==null&&i==null?(i=n,r=n=t):i==null&&(typeof n=="string"?(i=r,r=t):(i=r,r=n,n=t));if(i===!1)i=et;else if(!i)return this;return s===1&&(o=i,i=function(e){return v().off(e),o.apply(this,arguments)},i.guid=o.guid||(o.guid=v.guid++)),this.each(function(){v.event.add(this,e,i,r,n)})},one:function(e,t,n,r){return this.on(e,t,n,r,1)},off:function(e,n,r){var i,s;if(e&&e.preventDefault&&e.handleObj)return i=e.handleObj,v(e.delegateTarget).off(i.namespace?i.origType+"."+i.namespace:i.origType,i.selector,i.handler),this;if(typeof e=="object"){for(s in e)this.off(s,n,e[s]);return this}if(n===!1||typeof n=="function")r=n,n=t;return r===!1&&(r=et),this.each(function(){v.event.remove(this,e,r,n)})},bind:function(e,t,n){return this.on(e,null,t,n)},unbind:function(e,t){return this.off(e,null,t)},live:function(e,t,n){return v(this.context).on(e,this.selector,t,n),this},die:function(e,t){return v(this.context).off(e,this.selector||"**",t),this},delegate:function(e,t,n,r){return this.on(t,e,n,r)},undelegate:function(e,t,n){return arguments.length===1?this.off(e,"**"):this.off(t,e||"**",n)},trigger:function(e,t){return this.each(function(){v.event.trigger(e,t,this)})},triggerHandler:function(e,t){if(this[0])return v.event.trigger(e,t,this[0],!0)},toggle:function(e){var t=arguments,n=e.guid||v.guid++,r=0,i=function(n){var i=(v._data(this,"lastToggle"+e.guid)||0)%r;return v._data(this,"lastToggle"+e.guid,i+1),n.preventDefault(),t[i].apply(this,arguments)||!1};i.guid=n;while(r<t.length)t[r++].guid=n;return this.click(i)},hover:function(e,t){return this.mouseenter(e).mouseleave(t||e)}}),v.each("blur focus focusin focusout load resize scroll unload click dblclick mousedown mouseup mousemove mouseover mouseout mouseenter mouseleave change select submit keydown keypress keyup error contextmenu".split(" "),function(e,t){v.fn[t]=function(e,n){return n==null&&(n=e,e=null),arguments.length>0?this.on(t,null,e,n):this.trigger(t)},Q.test(t)&&(v.event.fixHooks[t]=v.event.keyHooks),G.test(t)&&(v.event.fixHooks[t]=v.event.mouseHooks)}),function(e,t){function nt(e,t,n,r){n=n||[],t=t||g;var i,s,a,f,l=t.nodeType;if(!e||typeof e!="string")return n;if(l!==1&&l!==9)return[];a=o(t);if(!a&&!r)if(i=R.exec(e))if(f=i[1]){if(l===9){s=t.getElementById(f);if(!s||!s.parentNode)return n;if(s.id===f)return n.push(s),n}else if(t.ownerDocument&&(s=t.ownerDocument.getElementById(f))&&u(t,s)&&s.id===f)return n.push(s),n}else{if(i[2])return S.apply(n,x.call(t.getElementsByTagName(e),0)),n;if((f=i[3])&&Z&&t.getElementsByClassName)return S.apply(n,x.call(t.getElementsByClassName(f),0)),n}return vt(e.replace(j,"$1"),t,n,r,a)}function rt(e){return function(t){var n=t.nodeName.toLowerCase();return n==="input"&&t.type===e}}function it(e){return function(t){var n=t.nodeName.toLowerCase();return(n==="input"||n==="button")&&t.type===e}}function st(e){return N(function(t){return t=+t,N(function(n,r){var i,s=e([],n.length,t),o=s.length;while(o--)n[i=s[o]]&&(n[i]=!(r[i]=n[i]))})})}function ot(e,t,n){if(e===t)return n;var r=e.nextSibling;while(r){if(r===t)return-1;r=r.nextSibling}return 1}function ut(e,t){var n,r,s,o,u,a,f,l=L[d][e+" "];if(l)return t?0:l.slice(0);u=e,a=[],f=i.preFilter;while(u){if(!n||(r=F.exec(u)))r&&(u=u.slice(r[0].length)||u),a.push(s=[]);n=!1;if(r=I.exec(u))s.push(n=new m(r.shift())),u=u.slice(n.length),n.type=r[0].replace(j," ");for(o in i.filter)(r=J[o].exec(u))&&(!f[o]||(r=f[o](r)))&&(s.push(n=new m(r.shift())),u=u.slice(n.length),n.type=o,n.matches=r);if(!n)break}return t?u.length:u?nt.error(e):L(e,a).slice(0)}function at(e,t,r){var i=t.dir,s=r&&t.dir==="parentNode",o=w++;return t.first?function(t,n,r){while(t=t[i])if(s||t.nodeType===1)return e(t,n,r)}:function(t,r,u){if(!u){var a,f=b+" "+o+" ",l=f+n;while(t=t[i])if(s||t.nodeType===1){if((a=t[d])===l)return t.sizset;if(typeof a=="string"&&a.indexOf(f)===0){if(t.sizset)return t}else{t[d]=l;if(e(t,r,u))return t.sizset=!0,t;t.sizset=!1}}}else while(t=t[i])if(s||t.nodeType===1)if(e(t,r,u))return t}}function ft(e){return e.length>1?function(t,n,r){var i=e.length;while(i--)if(!e[i](t,n,r))return!1;return!0}:e[0]}function lt(e,t,n,r,i){var s,o=[],u=0,a=e.length,f=t!=null;for(;u<a;u++)if(s=e[u])if(!n||n(s,r,i))o.push(s),f&&t.push(u);return o}function ct(e,t,n,r,i,s){return r&&!r[d]&&(r=ct(r)),i&&!i[d]&&(i=ct(i,s)),N(function(s,o,u,a){var f,l,c,h=[],p=[],d=o.length,v=s||dt(t||"*",u.nodeType?[u]:u,[]),m=e&&(s||!t)?lt(v,h,e,u,a):v,g=n?i||(s?e:d||r)?[]:o:m;n&&n(m,g,u,a);if(r){f=lt(g,p),r(f,[],u,a),l=f.length;while(l--)if(c=f[l])g[p[l]]=!(m[p[l]]=c)}if(s){if(i||e){if(i){f=[],l=g.length;while(l--)(c=g[l])&&f.push(m[l]=c);i(null,g=[],f,a)}l=g.length;while(l--)(c=g[l])&&(f=i?T.call(s,c):h[l])>-1&&(s[f]=!(o[f]=c))}}else g=lt(g===o?g.splice(d,g.length):g),i?i(null,o,g,a):S.apply(o,g)})}function ht(e){var t,n,r,s=e.length,o=i.relative[e[0].type],u=o||i.relative[" "],a=o?1:0,f=at(function(e){return e===t},u,!0),l=at(function(e){return T.call(t,e)>-1},u,!0),h=[function(e,n,r){return!o&&(r||n!==c)||((t=n).nodeType?f(e,n,r):l(e,n,r))}];for(;a<s;a++)if(n=i.relative[e[a].type])h=[at(ft(h),n)];else{n=i.filter[e[a].type].apply(null,e[a].matches);if(n[d]){r=++a;for(;r<s;r++)if(i.relative[e[r].type])break;return ct(a>1&&ft(h),a>1&&e.slice(0,a-1).join("").replace(j,"$1"),n,a<r&&ht(e.slice(a,r)),r<s&&ht(e=e.slice(r)),r<s&&e.join(""))}h.push(n)}return ft(h)}function pt(e,t){var r=t.length>0,s=e.length>0,o=function(u,a,f,l,h){var p,d,v,m=[],y=0,w="0",x=u&&[],T=h!=null,N=c,C=u||s&&i.find.TAG("*",h&&a.parentNode||a),k=b+=N==null?1:Math.E;T&&(c=a!==g&&a,n=o.el);for(;(p=C[w])!=null;w++){if(s&&p){for(d=0;v=e[d];d++)if(v(p,a,f)){l.push(p);break}T&&(b=k,n=++o.el)}r&&((p=!v&&p)&&y--,u&&x.push(p))}y+=w;if(r&&w!==y){for(d=0;v=t[d];d++)v(x,m,a,f);if(u){if(y>0)while(w--)!x[w]&&!m[w]&&(m[w]=E.call(l));m=lt(m)}S.apply(l,m),T&&!u&&m.length>0&&y+t.length>1&&nt.uniqueSort(l)}return T&&(b=k,c=N),x};return o.el=0,r?N(o):o}function dt(e,t,n){var r=0,i=t.length;for(;r<i;r++)nt(e,t[r],n);return n}function vt(e,t,n,r,s){var o,u,f,l,c,h=ut(e),p=h.length;if(!r&&h.length===1){u=h[0]=h[0].slice(0);if(u.length>2&&(f=u[0]).type==="ID"&&t.nodeType===9&&!s&&i.relative[u[1].type]){t=i.find.ID(f.matches[0].replace($,""),t,s)[0];if(!t)return n;e=e.slice(u.shift().length)}for(o=J.POS.test(e)?-1:u.length-1;o>=0;o--){f=u[o];if(i.relative[l=f.type])break;if(c=i.find[l])if(r=c(f.matches[0].replace($,""),z.test(u[0].type)&&t.parentNode||t,s)){u.splice(o,1),e=r.length&&u.join("");if(!e)return S.apply(n,x.call(r,0)),n;break}}}return a(e,h)(r,t,s,n,z.test(e)),n}function mt(){}var n,r,i,s,o,u,a,f,l,c,h=!0,p="undefined",d=("sizcache"+Math.random()).replace(".",""),m=String,g=e.document,y=g.documentElement,b=0,w=0,E=[].pop,S=[].push,x=[].slice,T=[].indexOf||function(e){var t=0,n=this.length;for(;t<n;t++)if(this[t]===e)return t;return-1},N=function(e,t){return e[d]=t==null||t,e},C=function(){var e={},t=[];return N(function(n,r){return t.push(n)>i.cacheLength&&delete e[t.shift()],e[n+" "]=r},e)},k=C(),L=C(),A=C(),O="[\\x20\\t\\r\\n\\f]",M="(?:\\\\.|[-\\w]|[^\\x00-\\xa0])+",_=M.replace("w","w#"),D="([*^$|!~]?=)",P="\\["+O+"*("+M+")"+O+"*(?:"+D+O+"*(?:(['\"])((?:\\\\.|[^\\\\])*?)\\3|("+_+")|)|)"+O+"*\\]",H=":("+M+")(?:\\((?:(['\"])((?:\\\\.|[^\\\\])*?)\\2|([^()[\\]]*|(?:(?:"+P+")|[^:]|\\\\.)*|.*))\\)|)",B=":(even|odd|eq|gt|lt|nth|first|last)(?:\\("+O+"*((?:-\\d)?\\d*)"+O+"*\\)|)(?=[^-]|$)",j=new RegExp("^"+O+"+|((?:^|[^\\\\])(?:\\\\.)*)"+O+"+$","g"),F=new RegExp("^"+O+"*,"+O+"*"),I=new RegExp("^"+O+"*([\\x20\\t\\r\\n\\f>+~])"+O+"*"),q=new RegExp(H),R=/^(?:#([\w\-]+)|(\w+)|\.([\w\-]+))$/,U=/^:not/,z=/[\x20\t\r\n\f]*[+~]/,W=/:not\($/,X=/h\d/i,V=/input|select|textarea|button/i,$=/\\(?!\\)/g,J={ID:new RegExp("^#("+M+")"),CLASS:new RegExp("^\\.("+M+")"),NAME:new RegExp("^\\[name=['\"]?("+M+")['\"]?\\]"),TAG:new RegExp("^("+M.replace("w","w*")+")"),ATTR:new RegExp("^"+P),PSEUDO:new RegExp("^"+H),POS:new RegExp(B,"i"),CHILD:new RegExp("^:(only|nth|first|last)-child(?:\\("+O+"*(even|odd|(([+-]|)(\\d*)n|)"+O+"*(?:([+-]|)"+O+"*(\\d+)|))"+O+"*\\)|)","i"),needsContext:new RegExp("^"+O+"*[>+~]|"+B,"i")},K=function(e){var t=g.createElement("div");try{return e(t)}catch(n){return!1}finally{t=null}},Q=K(function(e){return e.appendChild(g.createComment("")),!e.getElementsByTagName("*").length}),G=K(function(e){return e.innerHTML="<a href='#'></a>",e.firstChild&&typeof e.firstChild.getAttribute!==p&&e.firstChild.getAttribute("href")==="#"}),Y=K(function(e){e.innerHTML="<select></select>";var t=typeof e.lastChild.getAttribute("multiple");return t!=="boolean"&&t!=="string"}),Z=K(function(e){return e.innerHTML="<div class='hidden e'></div><div class='hidden'></div>",!e.getElementsByClassName||!e.getElementsByClassName("e").length?!1:(e.lastChild.className="e",e.getElementsByClassName("e").length===2)}),et=K(function(e){e.id=d+0,e.innerHTML="<a name='"+d+"'></a><div name='"+d+"'></div>",y.insertBefore(e,y.firstChild);var t=g.getElementsByName&&g.getElementsByName(d).length===2+g.getElementsByName(d+0).length;return r=!g.getElementById(d),y.removeChild(e),t});try{x.call(y.childNodes,0)[0].nodeType}catch(tt){x=function(e){var t,n=[];for(;t=this[e];e++)n.push(t);return n}}nt.matches=function(e,t){return nt(e,null,null,t)},nt.matchesSelector=function(e,t){return nt(t,null,null,[e]).length>0},s=nt.getText=function(e){var t,n="",r=0,i=e.nodeType;if(i){if(i===1||i===9||i===11){if(typeof e.textContent=="string")return e.textContent;for(e=e.firstChild;e;e=e.nextSibling)n+=s(e)}else if(i===3||i===4)return e.nodeValue}else for(;t=e[r];r++)n+=s(t);return n},o=nt.isXML=function(e){var t=e&&(e.ownerDocument||e).documentElement;return t?t.nodeName!=="HTML":!1},u=nt.contains=y.contains?function(e,t){var n=e.nodeType===9?e.documentElement:e,r=t&&t.parentNode;return e===r||!!(r&&r.nodeType===1&&n.contains&&n.contains(r))}:y.compareDocumentPosition?function(e,t){return t&&!!(e.compareDocumentPosition(t)&16)}:function(e,t){while(t=t.parentNode)if(t===e)return!0;return!1},nt.attr=function(e,t){var n,r=o(e);return r||(t=t.toLowerCase()),(n=i.attrHandle[t])?n(e):r||Y?e.getAttribute(t):(n=e.getAttributeNode(t),n?typeof e[t]=="boolean"?e[t]?t:null:n.specified?n.value:null:null)},i=nt.selectors={cacheLength:50,createPseudo:N,match:J,attrHandle:G?{}:{href:function(e){return e.getAttribute("href",2)},type:function(e){return e.getAttribute("type")}},find:{ID:r?function(e,t,n){if(typeof t.getElementById!==p&&!n){var r=t.getElementById(e);return r&&r.parentNode?[r]:[]}}:function(e,n,r){if(typeof n.getElementById!==p&&!r){var i=n.getElementById(e);return i?i.id===e||typeof i.getAttributeNode!==p&&i.getAttributeNode("id").value===e?[i]:t:[]}},TAG:Q?function(e,t){if(typeof t.getElementsByTagName!==p)return t.getElementsByTagName(e)}:function(e,t){var n=t.getElementsByTagName(e);if(e==="*"){var r,i=[],s=0;for(;r=n[s];s++)r.nodeType===1&&i.push(r);return i}return n},NAME:et&&function(e,t){if(typeof t.getElementsByName!==p)return t.getElementsByName(name)},CLASS:Z&&function(e,t,n){if(typeof t.getElementsByClassName!==p&&!n)return t.getElementsByClassName(e)}},relative:{">":{dir:"parentNode",first:!0}," ":{dir:"parentNode"},"+":{dir:"previousSibling",first:!0},"~":{dir:"previousSibling"}},preFilter:{ATTR:function(e){return e[1]=e[1].replace($,""),e[3]=(e[4]||e[5]||"").replace($,""),e[2]==="~="&&(e[3]=" "+e[3]+" "),e.slice(0,4)},CHILD:function(e){return e[1]=e[1].toLowerCase(),e[1]==="nth"?(e[2]||nt.error(e[0]),e[3]=+(e[3]?e[4]+(e[5]||1):2*(e[2]==="even"||e[2]==="odd")),e[4]=+(e[6]+e[7]||e[2]==="odd")):e[2]&&nt.error(e[0]),e},PSEUDO:function(e){var t,n;if(J.CHILD.test(e[0]))return null;if(e[3])e[2]=e[3];else if(t=e[4])q.test(t)&&(n=ut(t,!0))&&(n=t.indexOf(")",t.length-n)-t.length)&&(t=t.slice(0,n),e[0]=e[0].slice(0,n)),e[2]=t;return e.slice(0,3)}},filter:{ID:r?function(e){return e=e.replace($,""),function(t){return t.getAttribute("id")===e}}:function(e){return e=e.replace($,""),function(t){var n=typeof t.getAttributeNode!==p&&t.getAttributeNode("id");return n&&n.value===e}},TAG:function(e){return e==="*"?function(){return!0}:(e=e.replace($,"").toLowerCase(),function(t){return t.nodeName&&t.nodeName.toLowerCase()===e})},CLASS:function(e){var t=k[d][e+" "];return t||(t=new RegExp("(^|"+O+")"+e+"("+O+"|$)"))&&k(e,function(e){return t.test(e.className||typeof e.getAttribute!==p&&e.getAttribute("class")||"")})},ATTR:function(e,t,n){return function(r,i){var s=nt.attr(r,e);return s==null?t==="!=":t?(s+="",t==="="?s===n:t==="!="?s!==n:t==="^="?n&&s.indexOf(n)===0:t==="*="?n&&s.indexOf(n)>-1:t==="$="?n&&s.substr(s.length-n.length)===n:t==="~="?(" "+s+" ").indexOf(n)>-1:t==="|="?s===n||s.substr(0,n.length+1)===n+"-":!1):!0}},CHILD:function(e,t,n,r){return e==="nth"?function(e){var t,i,s=e.parentNode;if(n===1&&r===0)return!0;if(s){i=0;for(t=s.firstChild;t;t=t.nextSibling)if(t.nodeType===1){i++;if(e===t)break}}return i-=r,i===n||i%n===0&&i/n>=0}:function(t){var n=t;switch(e){case"only":case"first":while(n=n.previousSibling)if(n.nodeType===1)return!1;if(e==="first")return!0;n=t;case"last":while(n=n.nextSibling)if(n.nodeType===1)return!1;return!0}}},PSEUDO:function(e,t){var n,r=i.pseudos[e]||i.setFilters[e.toLowerCase()]||nt.error("unsupported pseudo: "+e);return r[d]?r(t):r.length>1?(n=[e,e,"",t],i.setFilters.hasOwnProperty(e.toLowerCase())?N(function(e,n){var i,s=r(e,t),o=s.length;while(o--)i=T.call(e,s[o]),e[i]=!(n[i]=s[o])}):function(e){return r(e,0,n)}):r}},pseudos:{not:N(function(e){var t=[],n=[],r=a(e.replace(j,"$1"));return r[d]?N(function(e,t,n,i){var s,o=r(e,null,i,[]),u=e.length;while(u--)if(s=o[u])e[u]=!(t[u]=s)}):function(e,i,s){return t[0]=e,r(t,null,s,n),!n.pop()}}),has:N(function(e){return function(t){return nt(e,t).length>0}}),contains:N(function(e){return function(t){return(t.textContent||t.innerText||s(t)).indexOf(e)>-1}}),enabled:function(e){return e.disabled===!1},disabled:function(e){return e.disabled===!0},checked:function(e){var t=e.nodeName.toLowerCase();return t==="input"&&!!e.checked||t==="option"&&!!e.selected},selected:function(e){return e.parentNode&&e.parentNode.selectedIndex,e.selected===!0},parent:function(e){return!i.pseudos.empty(e)},empty:function(e){var t;e=e.firstChild;while(e){if(e.nodeName>"@"||(t=e.nodeType)===3||t===4)return!1;e=e.nextSibling}return!0},header:function(e){return X.test(e.nodeName)},text:function(e){var t,n;return e.nodeName.toLowerCase()==="input"&&(t=e.type)==="text"&&((n=e.getAttribute("type"))==null||n.toLowerCase()===t)},radio:rt("radio"),checkbox:rt("checkbox"),file:rt("file"),password:rt("password"),image:rt("image"),submit:it("submit"),reset:it("reset"),button:function(e){var t=e.nodeName.toLowerCase();return t==="input"&&e.type==="button"||t==="button"},input:function(e){return V.test(e.nodeName)},focus:function(e){var t=e.ownerDocument;return e===t.activeElement&&(!t.hasFocus||t.hasFocus())&&!!(e.type||e.href||~e.tabIndex)},active:function(e){return e===e.ownerDocument.activeElement},first:st(function(){return[0]}),last:st(function(e,t){return[t-1]}),eq:st(function(e,t,n){return[n<0?n+t:n]}),even:st(function(e,t){for(var n=0;n<t;n+=2)e.push(n);return e}),odd:st(function(e,t){for(var n=1;n<t;n+=2)e.push(n);return e}),lt:st(function(e,t,n){for(var r=n<0?n+t:n;--r>=0;)e.push(r);return e}),gt:st(function(e,t,n){for(var r=n<0?n+t:n;++r<t;)e.push(r);return e})}},f=y.compareDocumentPosition?function(e,t){return e===t?(l=!0,0):(!e.compareDocumentPosition||!t.compareDocumentPosition?e.compareDocumentPosition:e.compareDocumentPosition(t)&4)?-1:1}:function(e,t){if(e===t)return l=!0,0;if(e.sourceIndex&&t.sourceIndex)return e.sourceIndex-t.sourceIndex;var n,r,i=[],s=[],o=e.parentNode,u=t.parentNode,a=o;if(o===u)return ot(e,t);if(!o)return-1;if(!u)return 1;while(a)i.unshift(a),a=a.parentNode;a=u;while(a)s.unshift(a),a=a.parentNode;n=i.length,r=s.length;for(var f=0;f<n&&f<r;f++)if(i[f]!==s[f])return ot(i[f],s[f]);return f===n?ot(e,s[f],-1):ot(i[f],t,1)},[0,0].sort(f),h=!l,nt.uniqueSort=function(e){var t,n=[],r=1,i=0;l=h,e.sort(f);if(l){for(;t=e[r];r++)t===e[r-1]&&(i=n.push(r));while(i--)e.splice(n[i],1)}return e},nt.error=function(e){throw new Error("Syntax error, unrecognized expression: "+e)},a=nt.compile=function(e,t){var n,r=[],i=[],s=A[d][e+" "];if(!s){t||(t=ut(e)),n=t.length;while(n--)s=ht(t[n]),s[d]?r.push(s):i.push(s);s=A(e,pt(i,r))}return s},g.querySelectorAll&&function(){var e,t=vt,n=/'|\\/g,r=/\=[\x20\t\r\n\f]*([^'"\]]*)[\x20\t\r\n\f]*\]/g,i=[":focus"],s=[":active"],u=y.matchesSelector||y.mozMatchesSelector||y.webkitMatchesSelector||y.oMatchesSelector||y.msMatchesSelector;K(function(e){e.innerHTML="<select><option selected=''></option></select>",e.querySelectorAll("[selected]").length||i.push("\\["+O+"*(?:checked|disabled|ismap|multiple|readonly|selected|value)"),e.querySelectorAll(":checked").length||i.push(":checked")}),K(function(e){e.innerHTML="<p test=''></p>",e.querySelectorAll("[test^='']").length&&i.push("[*^$]="+O+"*(?:\"\"|'')"),e.innerHTML="<input type='hidden'/>",e.querySelectorAll(":enabled").length||i.push(":enabled",":disabled")}),i=new RegExp(i.join("|")),vt=function(e,r,s,o,u){if(!o&&!u&&!i.test(e)){var a,f,l=!0,c=d,h=r,p=r.nodeType===9&&e;if(r.nodeType===1&&r.nodeName.toLowerCase()!=="object"){a=ut(e),(l=r.getAttribute("id"))?c=l.replace(n,"\\$&"):r.setAttribute("id",c),c="[id='"+c+"'] ",f=a.length;while(f--)a[f]=c+a[f].join("");h=z.test(e)&&r.parentNode||r,p=a.join(",")}if(p)try{return S.apply(s,x.call(h.querySelectorAll(p),0)),s}catch(v){}finally{l||r.removeAttribute("id")}}return t(e,r,s,o,u)},u&&(K(function(t){e=u.call(t,"div");try{u.call(t,"[test!='']:sizzle"),s.push("!=",H)}catch(n){}}),s=new RegExp(s.join("|")),nt.matchesSelector=function(t,n){n=n.replace(r,"='$1']");if(!o(t)&&!s.test(n)&&!i.test(n))try{var a=u.call(t,n);if(a||e||t.document&&t.document.nodeType!==11)return a}catch(f){}return nt(n,null,null,[t]).length>0})}(),i.pseudos.nth=i.pseudos.eq,i.filters=mt.prototype=i.pseudos,i.setFilters=new mt,nt.attr=v.attr,v.find=nt,v.expr=nt.selectors,v.expr[":"]=v.expr.pseudos,v.unique=nt.uniqueSort,v.text=nt.getText,v.isXMLDoc=nt.isXML,v.contains=nt.contains}(e);var nt=/Until$/,rt=/^(?:parents|prev(?:Until|All))/,it=/^.[^:#\[\.,]*$/,st=v.expr.match.needsContext,ot={children:!0,contents:!0,next:!0,prev:!0};v.fn.extend({find:function(e){var t,n,r,i,s,o,u=this;if(typeof e!="string")return v(e).filter(function(){for(t=0,n=u.length;t<n;t++)if(v.contains(u[t],this))return!0});o=this.pushStack("","find",e);for(t=0,n=this.length;t<n;t++){r=o.length,v.find(e,this[t],o);if(t>0)for(i=r;i<o.length;i++)for(s=0;s<r;s++)if(o[s]===o[i]){o.splice(i--,1);break}}return o},has:function(e){var t,n=v(e,this),r=n.length;return this.filter(function(){for(t=0;t<r;t++)if(v.contains(this,n[t]))return!0})},not:function(e){return this.pushStack(ft(this,e,!1),"not",e)},filter:function(e){return this.pushStack(ft(this,e,!0),"filter",e)},is:function(e){return!!e&&(typeof e=="string"?st.test(e)?v(e,this.context).index(this[0])>=0:v.filter(e,this).length>0:this.filter(e).length>0)},closest:function(e,t){var n,r=0,i=this.length,s=[],o=st.test(e)||typeof e!="string"?v(e,t||this.context):0;for(;r<i;r++){n=this[r];while(n&&n.ownerDocument&&n!==t&&n.nodeType!==11){if(o?o.index(n)>-1:v.find.matchesSelector(n,e)){s.push(n);break}n=n.parentNode}}return s=s.length>1?v.unique(s):s,this.pushStack(s,"closest",e)},index:function(e){return e?typeof e=="string"?v.inArray(this[0],v(e)):v.inArray(e.jquery?e[0]:e,this):this[0]&&this[0].parentNode?this.prevAll().length:-1},add:function(e,t){var n=typeof e=="string"?v(e,t):v.makeArray(e&&e.nodeType?[e]:e),r=v.merge(this.get(),n);return this.pushStack(ut(n[0])||ut(r[0])?r:v.unique(r))},addBack:function(e){return this.add(e==null?this.prevObject:this.prevObject.filter(e))}}),v.fn.andSelf=v.fn.addBack,v.each({parent:function(e){var t=e.parentNode;return t&&t.nodeType!==11?t:null},parents:function(e){return v.dir(e,"parentNode")},parentsUntil:function(e,t,n){return v.dir(e,"parentNode",n)},next:function(e){return at(e,"nextSibling")},prev:function(e){return at(e,"previousSibling")},nextAll:function(e){return v.dir(e,"nextSibling")},prevAll:function(e){return v.dir(e,"previousSibling")},nextUntil:function(e,t,n){return v.dir(e,"nextSibling",n)},prevUntil:function(e,t,n){return v.dir(e,"previousSibling",n)},siblings:function(e){return v.sibling((e.parentNode||{}).firstChild,e)},children:function(e){return v.sibling(e.firstChild)},contents:function(e){return v.nodeName(e,"iframe")?e.contentDocument||e.contentWindow.document:v.merge([],e.childNodes)}},function(e,t){v.fn[e]=function(n,r){var i=v.map(this,t,n);return nt.test(e)||(r=n),r&&typeof r=="string"&&(i=v.filter(r,i)),i=this.length>1&&!ot[e]?v.unique(i):i,this.length>1&&rt.test(e)&&(i=i.reverse()),this.pushStack(i,e,l.call(arguments).join(","))}}),v.extend({filter:function(e,t,n){return n&&(e=":not("+e+")"),t.length===1?v.find.matchesSelector(t[0],e)?[t[0]]:[]:v.find.matches(e,t)},dir:function(e,n,r){var i=[],s=e[n];while(s&&s.nodeType!==9&&(r===t||s.nodeType!==1||!v(s).is(r)))s.nodeType===1&&i.push(s),s=s[n];return i},sibling:function(e,t){var n=[];for(;e;e=e.nextSibling)e.nodeType===1&&e!==t&&n.push(e);return n}});var ct="abbr|article|aside|audio|bdi|canvas|data|datalist|details|figcaption|figure|footer|header|hgroup|mark|meter|nav|output|progress|section|summary|time|video",ht=/ jQuery\d+="(?:null|\d+)"/g,pt=/^\s+/,dt=/<(?!area|br|col|embed|hr|img|input|link|meta|param)(([\w:]+)[^>]*)\/>/gi,vt=/<([\w:]+)/,mt=/<tbody/i,gt=/<|&#?\w+;/,yt=/<(?:script|style|link)/i,bt=/<(?:script|object|embed|option|style)/i,wt=new RegExp("<(?:"+ct+")[\\s/>]","i"),Et=/^(?:checkbox|radio)$/,St=/checked\s*(?:[^=]|=\s*.checked.)/i,xt=/\/(java|ecma)script/i,Tt=/^\s*<!(?:\[CDATA\[|\-\-)|[\]\-]{2}>\s*$/g,Nt={option:[1,"<select multiple='multiple'>","</select>"],legend:[1,"<fieldset>","</fieldset>"],thead:[1,"<table>","</table>"],tr:[2,"<table><tbody>","</tbody></table>"],td:[3,"<table><tbody><tr>","</tr></tbody></table>"],col:[2,"<table><tbody></tbody><colgroup>","</colgroup></table>"],area:[1,"<map>","</map>"],_default:[0,"",""]},Ct=lt(i),kt=Ct.appendChild(i.createElement("div"));Nt.optgroup=Nt.option,Nt.tbody=Nt.tfoot=Nt.colgroup=Nt.caption=Nt.thead,Nt.th=Nt.td,v.support.htmlSerialize||(Nt._default=[1,"X<div>","</div>"]),v.fn.extend({text:function(e){return v.access(this,function(e){return e===t?v.text(this):this.empty().append((this[0]&&this[0].ownerDocument||i).createTextNode(e))},null,e,arguments.length)},wrapAll:function(e){if(v.isFunction(e))return this.each(function(t){v(this).wrapAll(e.call(this,t))});if(this[0]){var t=v(e,this[0].ownerDocument).eq(0).clone(!0);this[0].parentNode&&t.insertBefore(this[0]),t.map(function(){var e=this;while(e.firstChild&&e.firstChild.nodeType===1)e=e.firstChild;return e}).append(this)}return this},wrapInner:function(e){return v.isFunction(e)?this.each(function(t){v(this).wrapInner(e.call(this,t))}):this.each(function(){var t=v(this),n=t.contents();n.length?n.wrapAll(e):t.append(e)})},wrap:function(e){var t=v.isFunction(e);return this.each(function(n){v(this).wrapAll(t?e.call(this,n):e)})},unwrap:function(){return this.parent().each(function(){v.nodeName(this,"body")||v(this).replaceWith(this.childNodes)}).end()},append:function(){return this.domManip(arguments,!0,function(e){(this.nodeType===1||this.nodeType===11)&&this.appendChild(e)})},prepend:function(){return this.domManip(arguments,!0,function(e){(this.nodeType===1||this.nodeType===11)&&this.insertBefore(e,this.firstChild)})},before:function(){if(!ut(this[0]))return this.domManip(arguments,!1,function(e){this.parentNode.insertBefore(e,this)});if(arguments.length){var e=v.clean(arguments);return this.pushStack(v.merge(e,this),"before",this.selector)}},after:function(){if(!ut(this[0]))return this.domManip(arguments,!1,function(e){this.parentNode.insertBefore(e,this.nextSibling)});if(arguments.length){var e=v.clean(arguments);return this.pushStack(v.merge(this,e),"after",this.selector)}},remove:function(e,t){var n,r=0;for(;(n=this[r])!=null;r++)if(!e||v.filter(e,[n]).length)!t&&n.nodeType===1&&(v.cleanData(n.getElementsByTagName("*")),v.cleanData([n])),n.parentNode&&n.parentNode.removeChild(n);return this},empty:function(){var e,t=0;for(;(e=this[t])!=null;t++){e.nodeType===1&&v.cleanData(e.getElementsByTagName("*"));while(e.firstChild)e.removeChild(e.firstChild)}return this},clone:function(e,t){return e=e==null?!1:e,t=t==null?e:t,this.map(function(){return v.clone(this,e,t)})},html:function(e){return v.access(this,function(e){var n=this[0]||{},r=0,i=this.length;if(e===t)return n.nodeType===1?n.innerHTML.replace(ht,""):t;if(typeof e=="string"&&!yt.test(e)&&(v.support.htmlSerialize||!wt.test(e))&&(v.support.leadingWhitespace||!pt.test(e))&&!Nt[(vt.exec(e)||["",""])[1].toLowerCase()]){e=e.replace(dt,"<$1></$2>");try{for(;r<i;r++)n=this[r]||{},n.nodeType===1&&(v.cleanData(n.getElementsByTagName("*")),n.innerHTML=e);n=0}catch(s){}}n&&this.empty().append(e)},null,e,arguments.length)},replaceWith:function(e){return ut(this[0])?this.length?this.pushStack(v(v.isFunction(e)?e():e),"replaceWith",e):this:v.isFunction(e)?this.each(function(t){var n=v(this),r=n.html();n.replaceWith(e.call(this,t,r))}):(typeof e!="string"&&(e=v(e).detach()),this.each(function(){var t=this.nextSibling,n=this.parentNode;v(this).remove(),t?v(t).before(e):v(n).append(e)}))},detach:function(e){return this.remove(e,!0)},domManip:function(e,n,r){e=[].concat.apply([],e);var i,s,o,u,a=0,f=e[0],l=[],c=this.length;if(!v.support.checkClone&&c>1&&typeof f=="string"&&St.test(f))return this.each(function(){v(this).domManip(e,n,r)});if(v.isFunction(f))return this.each(function(i){var s=v(this);e[0]=f.call(this,i,n?s.html():t),s.domManip(e,n,r)});if(this[0]){i=v.buildFragment(e,this,l),o=i.fragment,s=o.firstChild,o.childNodes.length===1&&(o=s);if(s){n=n&&v.nodeName(s,"tr");for(u=i.cacheable||c-1;a<c;a++)r.call(n&&v.nodeName(this[a],"table")?Lt(this[a],"tbody"):this[a],a===u?o:v.clone(o,!0,!0))}o=s=null,l.length&&v.each(l,function(e,t){t.src?v.ajax?v.ajax({url:t.src,type:"GET",dataType:"script",async:!1,global:!1,"throws":!0}):v.error("no ajax"):v.globalEval((t.text||t.textContent||t.innerHTML||"").replace(Tt,"")),t.parentNode&&t.parentNode.removeChild(t)})}return this}}),v.buildFragment=function(e,n,r){var s,o,u,a=e[0];return n=n||i,n=!n.nodeType&&n[0]||n,n=n.ownerDocument||n,e.length===1&&typeof a=="string"&&a.length<512&&n===i&&a.charAt(0)==="<"&&!bt.test(a)&&(v.support.checkClone||!St.test(a))&&(v.support.html5Clone||!wt.test(a))&&(o=!0,s=v.fragments[a],u=s!==t),s||(s=n.createDocumentFragment(),v.clean(e,n,s,r),o&&(v.fragments[a]=u&&s)),{fragment:s,cacheable:o}},v.fragments={},v.each({appendTo:"append",prependTo:"prepend",insertBefore:"before",insertAfter:"after",replaceAll:"replaceWith"},function(e,t){v.fn[e]=function(n){var r,i=0,s=[],o=v(n),u=o.length,a=this.length===1&&this[0].parentNode;if((a==null||a&&a.nodeType===11&&a.childNodes.length===1)&&u===1)return o[t](this[0]),this;for(;i<u;i++)r=(i>0?this.clone(!0):this).get(),v(o[i])[t](r),s=s.concat(r);return this.pushStack(s,e,o.selector)}}),v.extend({clone:function(e,t,n){var r,i,s,o;v.support.html5Clone||v.isXMLDoc(e)||!wt.test("<"+e.nodeName+">")?o=e.cloneNode(!0):(kt.innerHTML=e.outerHTML,kt.removeChild(o=kt.firstChild));if((!v.support.noCloneEvent||!v.support.noCloneChecked)&&(e.nodeType===1||e.nodeType===11)&&!v.isXMLDoc(e)){Ot(e,o),r=Mt(e),i=Mt(o);for(s=0;r[s];++s)i[s]&&Ot(r[s],i[s])}if(t){At(e,o);if(n){r=Mt(e),i=Mt(o);for(s=0;r[s];++s)At(r[s],i[s])}}return r=i=null,o},clean:function(e,t,n,r){var s,o,u,a,f,l,c,h,p,d,m,g,y=t===i&&Ct,b=[];if(!t||typeof t.createDocumentFragment=="undefined")t=i;for(s=0;(u=e[s])!=null;s++){typeof u=="number"&&(u+="");if(!u)continue;if(typeof u=="string")if(!gt.test(u))u=t.createTextNode(u);else{y=y||lt(t),c=t.createElement("div"),y.appendChild(c),u=u.replace(dt,"<$1></$2>"),a=(vt.exec(u)||["",""])[1].toLowerCase(),f=Nt[a]||Nt._default,l=f[0],c.innerHTML=f[1]+u+f[2];while(l--)c=c.lastChild;if(!v.support.tbody){h=mt.test(u),p=a==="table"&&!h?c.firstChild&&c.firstChild.childNodes:f[1]==="<table>"&&!h?c.childNodes:[];for(o=p.length-1;o>=0;--o)v.nodeName(p[o],"tbody")&&!p[o].childNodes.length&&p[o].parentNode.removeChild(p[o])}!v.support.leadingWhitespace&&pt.test(u)&&c.insertBefore(t.createTextNode(pt.exec(u)[0]),c.firstChild),u=c.childNodes,c.parentNode.removeChild(c)}u.nodeType?b.push(u):v.merge(b,u)}c&&(u=c=y=null);if(!v.support.appendChecked)for(s=0;(u=b[s])!=null;s++)v.nodeName(u,"input")?_t(u):typeof u.getElementsByTagName!="undefined"&&v.grep(u.getElementsByTagName("input"),_t);if(n){m=function(e){if(!e.type||xt.test(e.type))return r?r.push(e.parentNode?e.parentNode.removeChild(e):e):n.appendChild(e)};for(s=0;(u=b[s])!=null;s++)if(!v.nodeName(u,"script")||!m(u))n.appendChild(u),typeof u.getElementsByTagName!="undefined"&&(g=v.grep(v.merge([],u.getElementsByTagName("script")),m),b.splice.apply(b,[s+1,0].concat(g)),s+=g.length)}return b},cleanData:function(e,t){var n,r,i,s,o=0,u=v.expando,a=v.cache,f=v.support.deleteExpando,l=v.event.special;for(;(i=e[o])!=null;o++)if(t||v.acceptData(i)){r=i[u],n=r&&a[r];if(n){if(n.events)for(s in n.events)l[s]?v.event.remove(i,s):v.removeEvent(i,s,n.handle);a[r]&&(delete a[r],f?delete i[u]:i.removeAttribute?i.removeAttribute(u):i[u]=null,v.deletedIds.push(r))}}}}),function(){var e,t;v.uaMatch=function(e){e=e.toLowerCase();var t=/(chrome)[ \/]([\w.]+)/.exec(e)||/(webkit)[ \/]([\w.]+)/.exec(e)||/(opera)(?:.*version|)[ \/]([\w.]+)/.exec(e)||/(msie) ([\w.]+)/.exec(e)||e.indexOf("compatible")<0&&/(mozilla)(?:.*? rv:([\w.]+)|)/.exec(e)||[];return{browser:t[1]||"",version:t[2]||"0"}},e=v.uaMatch(o.userAgent),t={},e.browser&&(t[e.browser]=!0,t.version=e.version),t.chrome?t.webkit=!0:t.webkit&&(t.safari=!0),v.browser=t,v.sub=function(){function e(t,n){return new e.fn.init(t,n)}v.extend(!0,e,this),e.superclass=this,e.fn=e.prototype=this(),e.fn.constructor=e,e.sub=this.sub,e.fn.init=function(r,i){return i&&i instanceof v&&!(i instanceof e)&&(i=e(i)),v.fn.init.call(this,r,i,t)},e.fn.init.prototype=e.fn;var t=e(i);return e}}();var Dt,Pt,Ht,Bt=/alpha\([^)]*\)/i,jt=/opacity=([^)]*)/,Ft=/^(top|right|bottom|left)$/,It=/^(none|table(?!-c[ea]).+)/,qt=/^margin/,Rt=new RegExp("^("+m+")(.*)$","i"),Ut=new RegExp("^("+m+")(?!px)[a-z%]+$","i"),zt=new RegExp("^([-+])=("+m+")","i"),Wt={BODY:"block"},Xt={position:"absolute",visibility:"hidden",display:"block"},Vt={letterSpacing:0,fontWeight:400},$t=["Top","Right","Bottom","Left"],Jt=["Webkit","O","Moz","ms"],Kt=v.fn.toggle;v.fn.extend({css:function(e,n){return v.access(this,function(e,n,r){return r!==t?v.style(e,n,r):v.css(e,n)},e,n,arguments.length>1)},show:function(){return Yt(this,!0)},hide:function(){return Yt(this)},toggle:function(e,t){var n=typeof e=="boolean";return v.isFunction(e)&&v.isFunction(t)?Kt.apply(this,arguments):this.each(function(){(n?e:Gt(this))?v(this).show():v(this).hide()})}}),v.extend({cssHooks:{opacity:{get:function(e,t){if(t){var n=Dt(e,"opacity");return n===""?"1":n}}}},cssNumber:{fillOpacity:!0,fontWeight:!0,lineHeight:!0,opacity:!0,orphans:!0,widows:!0,zIndex:!0,zoom:!0},cssProps:{"float":v.support.cssFloat?"cssFloat":"styleFloat"},style:function(e,n,r,i){if(!e||e.nodeType===3||e.nodeType===8||!e.style)return;var s,o,u,a=v.camelCase(n),f=e.style;n=v.cssProps[a]||(v.cssProps[a]=Qt(f,a)),u=v.cssHooks[n]||v.cssHooks[a];if(r===t)return u&&"get"in u&&(s=u.get(e,!1,i))!==t?s:f[n];o=typeof r,o==="string"&&(s=zt.exec(r))&&(r=(s[1]+1)*s[2]+parseFloat(v.css(e,n)),o="number");if(r==null||o==="number"&&isNaN(r))return;o==="number"&&!v.cssNumber[a]&&(r+="px");if(!u||!("set"in u)||(r=u.set(e,r,i))!==t)try{f[n]=r}catch(l){}},css:function(e,n,r,i){var s,o,u,a=v.camelCase(n);return n=v.cssProps[a]||(v.cssProps[a]=Qt(e.style,a)),u=v.cssHooks[n]||v.cssHooks[a],u&&"get"in u&&(s=u.get(e,!0,i)),s===t&&(s=Dt(e,n)),s==="normal"&&n in Vt&&(s=Vt[n]),r||i!==t?(o=parseFloat(s),r||v.isNumeric(o)?o||0:s):s},swap:function(e,t,n){var r,i,s={};for(i in t)s[i]=e.style[i],e.style[i]=t[i];r=n.call(e);for(i in t)e.style[i]=s[i];return r}}),e.getComputedStyle?Dt=function(t,n){var r,i,s,o,u=e.getComputedStyle(t,null),a=t.style;return u&&(r=u.getPropertyValue(n)||u[n],r===""&&!v.contains(t.ownerDocument,t)&&(r=v.style(t,n)),Ut.test(r)&&qt.test(n)&&(i=a.width,s=a.minWidth,o=a.maxWidth,a.minWidth=a.maxWidth=a.width=r,r=u.width,a.width=i,a.minWidth=s,a.maxWidth=o)),r}:i.documentElement.currentStyle&&(Dt=function(e,t){var n,r,i=e.currentStyle&&e.currentStyle[t],s=e.style;return i==null&&s&&s[t]&&(i=s[t]),Ut.test(i)&&!Ft.test(t)&&(n=s.left,r=e.runtimeStyle&&e.runtimeStyle.left,r&&(e.runtimeStyle.left=e.currentStyle.left),s.left=t==="fontSize"?"1em":i,i=s.pixelLeft+"px",s.left=n,r&&(e.runtimeStyle.left=r)),i===""?"auto":i}),v.each(["height","width"],function(e,t){v.cssHooks[t]={get:function(e,n,r){if(n)return e.offsetWidth===0&&It.test(Dt(e,"display"))?v.swap(e,Xt,function(){return tn(e,t,r)}):tn(e,t,r)},set:function(e,n,r){return Zt(e,n,r?en(e,t,r,v.support.boxSizing&&v.css(e,"boxSizing")==="border-box"):0)}}}),v.support.opacity||(v.cssHooks.opacity={get:function(e,t){return jt.test((t&&e.currentStyle?e.currentStyle.filter:e.style.filter)||"")?.01*parseFloat(RegExp.$1)+"":t?"1":""},set:function(e,t){var n=e.style,r=e.currentStyle,i=v.isNumeric(t)?"alpha(opacity="+t*100+")":"",s=r&&r.filter||n.filter||"";n.zoom=1;if(t>=1&&v.trim(s.replace(Bt,""))===""&&n.removeAttribute){n.removeAttribute("filter");if(r&&!r.filter)return}n.filter=Bt.test(s)?s.replace(Bt,i):s+" "+i}}),v(function(){v.support.reliableMarginRight||(v.cssHooks.marginRight={get:function(e,t){return v.swap(e,{display:"inline-block"},function(){if(t)return Dt(e,"marginRight")})}}),!v.support.pixelPosition&&v.fn.position&&v.each(["top","left"],function(e,t){v.cssHooks[t]={get:function(e,n){if(n){var r=Dt(e,t);return Ut.test(r)?v(e).position()[t]+"px":r}}}})}),v.expr&&v.expr.filters&&(v.expr.filters.hidden=function(e){return e.offsetWidth===0&&e.offsetHeight===0||!v.support.reliableHiddenOffsets&&(e.style&&e.style.display||Dt(e,"display"))==="none"},v.expr.filters.visible=function(e){return!v.expr.filters.hidden(e)}),v.each({margin:"",padding:"",border:"Width"},function(e,t){v.cssHooks[e+t]={expand:function(n){var r,i=typeof n=="string"?n.split(" "):[n],s={};for(r=0;r<4;r++)s[e+$t[r]+t]=i[r]||i[r-2]||i[0];return s}},qt.test(e)||(v.cssHooks[e+t].set=Zt)});var rn=/%20/g,sn=/\[\]$/,on=/\r?\n/g,un=/^(?:color|date|datetime|datetime-local|email|hidden|month|number|password|range|search|tel|text|time|url|week)$/i,an=/^(?:select|textarea)/i;v.fn.extend({serialize:function(){return v.param(this.serializeArray())},serializeArray:function(){return this.map(function(){return this.elements?v.makeArray(this.elements):this}).filter(function(){return this.name&&!this.disabled&&(this.checked||an.test(this.nodeName)||un.test(this.type))}).map(function(e,t){var n=v(this).val();return n==null?null:v.isArray(n)?v.map(n,function(e,n){return{name:t.name,value:e.replace(on,"\r\n")}}):{name:t.name,value:n.replace(on,"\r\n")}}).get()}}),v.param=function(e,n){var r,i=[],s=function(e,t){t=v.isFunction(t)?t():t==null?"":t,i[i.length]=encodeURIComponent(e)+"="+encodeURIComponent(t)};n===t&&(n=v.ajaxSettings&&v.ajaxSettings.traditional);if(v.isArray(e)||e.jquery&&!v.isPlainObject(e))v.each(e,function(){s(this.name,this.value)});else for(r in e)fn(r,e[r],n,s);return i.join("&").replace(rn,"+")};var ln,cn,hn=/#.*$/,pn=/^(.*?):[ \t]*([^\r\n]*)\r?$/mg,dn=/^(?:about|app|app\-storage|.+\-extension|file|res|widget):$/,vn=/^(?:GET|HEAD)$/,mn=/^\/\//,gn=/\?/,yn=/<script\b[^<]*(?:(?!<\/script>)<[^<]*)*<\/script>/gi,bn=/([?&])_=[^&]*/,wn=/^([\w\+\.\-]+:)(?:\/\/([^\/?#:]*)(?::(\d+)|)|)/,En=v.fn.load,Sn={},xn={},Tn=["*/"]+["*"];try{cn=s.href}catch(Nn){cn=i.createElement("a"),cn.href="",cn=cn.href}ln=wn.exec(cn.toLowerCase())||[],v.fn.load=function(e,n,r){if(typeof e!="string"&&En)return En.apply(this,arguments);if(!this.length)return this;var i,s,o,u=this,a=e.indexOf(" ");return a>=0&&(i=e.slice(a,e.length),e=e.slice(0,a)),v.isFunction(n)?(r=n,n=t):n&&typeof n=="object"&&(s="POST"),v.ajax({url:e,type:s,dataType:"html",data:n,complete:function(e,t){r&&u.each(r,o||[e.responseText,t,e])}}).done(function(e){o=arguments,u.html(i?v("<div>").append(e.replace(yn,"")).find(i):e)}),this},v.each("ajaxStart ajaxStop ajaxComplete ajaxError ajaxSuccess ajaxSend".split(" "),function(e,t){v.fn[t]=function(e){return this.on(t,e)}}),v.each(["get","post"],function(e,n){v[n]=function(e,r,i,s){return v.isFunction(r)&&(s=s||i,i=r,r=t),v.ajax({type:n,url:e,data:r,success:i,dataType:s})}}),v.extend({getScript:function(e,n){return v.get(e,t,n,"script")},getJSON:function(e,t,n){return v.get(e,t,n,"json")},ajaxSetup:function(e,t){return t?Ln(e,v.ajaxSettings):(t=e,e=v.ajaxSettings),Ln(e,t),e},ajaxSettings:{url:cn,isLocal:dn.test(ln[1]),global:!0,type:"GET",contentType:"application/x-www-form-urlencoded; charset=UTF-8",processData:!0,async:!0,accepts:{xml:"application/xml, text/xml",html:"text/html",text:"text/plain",json:"application/json, text/javascript","*":Tn},contents:{xml:/xml/,html:/html/,json:/json/},responseFields:{xml:"responseXML",text:"responseText"},converters:{"* text":e.String,"text html":!0,"text json":v.parseJSON,"text xml":v.parseXML},flatOptions:{context:!0,url:!0}},ajaxPrefilter:Cn(Sn),ajaxTransport:Cn(xn),ajax:function(e,n){function T(e,n,s,a){var l,y,b,w,S,T=n;if(E===2)return;E=2,u&&clearTimeout(u),o=t,i=a||"",x.readyState=e>0?4:0,s&&(w=An(c,x,s));if(e>=200&&e<300||e===304)c.ifModified&&(S=x.getResponseHeader("Last-Modified"),S&&(v.lastModified[r]=S),S=x.getResponseHeader("Etag"),S&&(v.etag[r]=S)),e===304?(T="notmodified",l=!0):(l=On(c,w),T=l.state,y=l.data,b=l.error,l=!b);else{b=T;if(!T||e)T="error",e<0&&(e=0)}x.status=e,x.statusText=(n||T)+"",l?d.resolveWith(h,[y,T,x]):d.rejectWith(h,[x,T,b]),x.statusCode(g),g=t,f&&p.trigger("ajax"+(l?"Success":"Error"),[x,c,l?y:b]),m.fireWith(h,[x,T]),f&&(p.trigger("ajaxComplete",[x,c]),--v.active||v.event.trigger("ajaxStop"))}typeof e=="object"&&(n=e,e=t),n=n||{};var r,i,s,o,u,a,f,l,c=v.ajaxSetup({},n),h=c.context||c,p=h!==c&&(h.nodeType||h instanceof v)?v(h):v.event,d=v.Deferred(),m=v.Callbacks("once memory"),g=c.statusCode||{},b={},w={},E=0,S="canceled",x={readyState:0,setRequestHeader:function(e,t){if(!E){var n=e.toLowerCase();e=w[n]=w[n]||e,b[e]=t}return this},getAllResponseHeaders:function(){return E===2?i:null},getResponseHeader:function(e){var n;if(E===2){if(!s){s={};while(n=pn.exec(i))s[n[1].toLowerCase()]=n[2]}n=s[e.toLowerCase()]}return n===t?null:n},overrideMimeType:function(e){return E||(c.mimeType=e),this},abort:function(e){return e=e||S,o&&o.abort(e),T(0,e),this}};d.promise(x),x.success=x.done,x.error=x.fail,x.complete=m.add,x.statusCode=function(e){if(e){var t;if(E<2)for(t in e)g[t]=[g[t],e[t]];else t=e[x.status],x.always(t)}return this},c.url=((e||c.url)+"").replace(hn,"").replace(mn,ln[1]+"//"),c.dataTypes=v.trim(c.dataType||"*").toLowerCase().split(y),c.crossDomain==null&&(a=wn.exec(c.url.toLowerCase()),c.crossDomain=!(!a||a[1]===ln[1]&&a[2]===ln[2]&&(a[3]||(a[1]==="http:"?80:443))==(ln[3]||(ln[1]==="http:"?80:443)))),c.data&&c.processData&&typeof c.data!="string"&&(c.data=v.param(c.data,c.traditional)),kn(Sn,c,n,x);if(E===2)return x;f=c.global,c.type=c.type.toUpperCase(),c.hasContent=!vn.test(c.type),f&&v.active++===0&&v.event.trigger("ajaxStart");if(!c.hasContent){c.data&&(c.url+=(gn.test(c.url)?"&":"?")+c.data,delete c.data),r=c.url;if(c.cache===!1){var N=v.now(),C=c.url.replace(bn,"$1_="+N);c.url=C+(C===c.url?(gn.test(c.url)?"&":"?")+"_="+N:"")}}(c.data&&c.hasContent&&c.contentType!==!1||n.contentType)&&x.setRequestHeader("Content-Type",c.contentType),c.ifModified&&(r=r||c.url,v.lastModified[r]&&x.setRequestHeader("If-Modified-Since",v.lastModified[r]),v.etag[r]&&x.setRequestHeader("If-None-Match",v.etag[r])),x.setRequestHeader("Accept",c.dataTypes[0]&&c.accepts[c.dataTypes[0]]?c.accepts[c.dataTypes[0]]+(c.dataTypes[0]!=="*"?", "+Tn+"; q=0.01":""):c.accepts["*"]);for(l in c.headers)x.setRequestHeader(l,c.headers[l]);if(!c.beforeSend||c.beforeSend.call(h,x,c)!==!1&&E!==2){S="abort";for(l in{success:1,error:1,complete:1})x[l](c[l]);o=kn(xn,c,n,x);if(!o)T(-1,"No Transport");else{x.readyState=1,f&&p.trigger("ajaxSend",[x,c]),c.async&&c.timeout>0&&(u=setTimeout(function(){x.abort("timeout")},c.timeout));try{E=1,o.send(b,T)}catch(k){if(!(E<2))throw k;T(-1,k)}}return x}return x.abort()},active:0,lastModified:{},etag:{}});var Mn=[],_n=/\?/,Dn=/(=)\?(?=&|$)|\?\?/,Pn=v.now();v.ajaxSetup({jsonp:"callback",jsonpCallback:function(){var e=Mn.pop()||v.expando+"_"+Pn++;return this[e]=!0,e}}),v.ajaxPrefilter("json jsonp",function(n,r,i){var s,o,u,a=n.data,f=n.url,l=n.jsonp!==!1,c=l&&Dn.test(f),h=l&&!c&&typeof a=="string"&&!(n.contentType||"").indexOf("application/x-www-form-urlencoded")&&Dn.test(a);if(n.dataTypes[0]==="jsonp"||c||h)return s=n.jsonpCallback=v.isFunction(n.jsonpCallback)?n.jsonpCallback():n.jsonpCallback,o=e[s],c?n.url=f.replace(Dn,"$1"+s):h?n.data=a.replace(Dn,"$1"+s):l&&(n.url+=(_n.test(f)?"&":"?")+n.jsonp+"="+s),n.converters["script json"]=function(){return u||v.error(s+" was not called"),u[0]},n.dataTypes[0]="json",e[s]=function(){u=arguments},i.always(function(){e[s]=o,n[s]&&(n.jsonpCallback=r.jsonpCallback,Mn.push(s)),u&&v.isFunction(o)&&o(u[0]),u=o=t}),"script"}),v.ajaxSetup({accepts:{script:"text/javascript, application/javascript, application/ecmascript, application/x-ecmascript"},contents:{script:/javascript|ecmascript/},converters:{"text script":function(e){return v.globalEval(e),e}}}),v.ajaxPrefilter("script",function(e){e.cache===t&&(e.cache=!1),e.crossDomain&&(e.type="GET",e.global=!1)}),v.ajaxTransport("script",function(e){if(e.crossDomain){var n,r=i.head||i.getElementsByTagName("head")[0]||i.documentElement;return{send:function(s,o){n=i.createElement("script"),n.async="async",e.scriptCharset&&(n.charset=e.scriptCharset),n.src=e.url,n.onload=n.onreadystatechange=function(e,i){if(i||!n.readyState||/loaded|complete/.test(n.readyState))n.onload=n.onreadystatechange=null,r&&n.parentNode&&r.removeChild(n),n=t,i||o(200,"success")},r.insertBefore(n,r.firstChild)},abort:function(){n&&n.onload(0,1)}}}});var Hn,Bn=e.ActiveXObject?function(){for(var e in Hn)Hn[e](0,1)}:!1,jn=0;v.ajaxSettings.xhr=e.ActiveXObject?function(){return!this.isLocal&&Fn()||In()}:Fn,function(e){v.extend(v.support,{ajax:!!e,cors:!!e&&"withCredentials"in e})}(v.ajaxSettings.xhr()),v.support.ajax&&v.ajaxTransport(function(n){if(!n.crossDomain||v.support.cors){var r;return{send:function(i,s){var o,u,a=n.xhr();n.username?a.open(n.type,n.url,n.async,n.username,n.password):a.open(n.type,n.url,n.async);if(n.xhrFields)for(u in n.xhrFields)a[u]=n.xhrFields[u];n.mimeType&&a.overrideMimeType&&a.overrideMimeType(n.mimeType),!n.crossDomain&&!i["X-Requested-With"]&&(i["X-Requested-With"]="XMLHttpRequest");try{for(u in i)a.setRequestHeader(u,i[u])}catch(f){}a.send(n.hasContent&&n.data||null),r=function(e,i){var u,f,l,c,h;try{if(r&&(i||a.readyState===4)){r=t,o&&(a.onreadystatechange=v.noop,Bn&&delete Hn[o]);if(i)a.readyState!==4&&a.abort();else{u=a.status,l=a.getAllResponseHeaders(),c={},h=a.responseXML,h&&h.documentElement&&(c.xml=h);try{c.text=a.responseText}catch(p){}try{f=a.statusText}catch(p){f=""}!u&&n.isLocal&&!n.crossDomain?u=c.text?200:404:u===1223&&(u=204)}}}catch(d){i||s(-1,d)}c&&s(u,f,c,l)},n.async?a.readyState===4?setTimeout(r,0):(o=++jn,Bn&&(Hn||(Hn={},v(e).unload(Bn)),Hn[o]=r),a.onreadystatechange=r):r()},abort:function(){r&&r(0,1)}}}});var qn,Rn,Un=/^(?:toggle|show|hide)$/,zn=new RegExp("^(?:([-+])=|)("+m+")([a-z%]*)$","i"),Wn=/queueHooks$/,Xn=[Gn],Vn={"*":[function(e,t){var n,r,i=this.createTween(e,t),s=zn.exec(t),o=i.cur(),u=+o||0,a=1,f=20;if(s){n=+s[2],r=s[3]||(v.cssNumber[e]?"":"px");if(r!=="px"&&u){u=v.css(i.elem,e,!0)||n||1;do a=a||".5",u/=a,v.style(i.elem,e,u+r);while(a!==(a=i.cur()/o)&&a!==1&&--f)}i.unit=r,i.start=u,i.end=s[1]?u+(s[1]+1)*n:n}return i}]};v.Animation=v.extend(Kn,{tweener:function(e,t){v.isFunction(e)?(t=e,e=["*"]):e=e.split(" ");var n,r=0,i=e.length;for(;r<i;r++)n=e[r],Vn[n]=Vn[n]||[],Vn[n].unshift(t)},prefilter:function(e,t){t?Xn.unshift(e):Xn.push(e)}}),v.Tween=Yn,Yn.prototype={constructor:Yn,init:function(e,t,n,r,i,s){this.elem=e,this.prop=n,this.easing=i||"swing",this.options=t,this.start=this.now=this.cur(),this.end=r,this.unit=s||(v.cssNumber[n]?"":"px")},cur:function(){var e=Yn.propHooks[this.prop];return e&&e.get?e.get(this):Yn.propHooks._default.get(this)},run:function(e){var t,n=Yn.propHooks[this.prop];return this.options.duration?this.pos=t=v.easing[this.easing](e,this.options.duration*e,0,1,this.options.duration):this.pos=t=e,this.now=(this.end-this.start)*t+this.start,this.options.step&&this.options.step.call(this.elem,this.now,this),n&&n.set?n.set(this):Yn.propHooks._default.set(this),this}},Yn.prototype.init.prototype=Yn.prototype,Yn.propHooks={_default:{get:function(e){var t;return e.elem[e.prop]==null||!!e.elem.style&&e.elem.style[e.prop]!=null?(t=v.css(e.elem,e.prop,!1,""),!t||t==="auto"?0:t):e.elem[e.prop]},set:function(e){v.fx.step[e.prop]?v.fx.step[e.prop](e):e.elem.style&&(e.elem.style[v.cssProps[e.prop]]!=null||v.cssHooks[e.prop])?v.style(e.elem,e.prop,e.now+e.unit):e.elem[e.prop]=e.now}}},Yn.propHooks.scrollTop=Yn.propHooks.scrollLeft={set:function(e){e.elem.nodeType&&e.elem.parentNode&&(e.elem[e.prop]=e.now)}},v.each(["toggle","show","hide"],function(e,t){var n=v.fn[t];v.fn[t]=function(r,i,s){return r==null||typeof r=="boolean"||!e&&v.isFunction(r)&&v.isFunction(i)?n.apply(this,arguments):this.animate(Zn(t,!0),r,i,s)}}),v.fn.extend({fadeTo:function(e,t,n,r){return this.filter(Gt).css("opacity",0).show().end().animate({opacity:t},e,n,r)},animate:function(e,t,n,r){var i=v.isEmptyObject(e),s=v.speed(t,n,r),o=function(){var t=Kn(this,v.extend({},e),s);i&&t.stop(!0)};return i||s.queue===!1?this.each(o):this.queue(s.queue,o)},stop:function(e,n,r){var i=function(e){var t=e.stop;delete e.stop,t(r)};return typeof e!="string"&&(r=n,n=e,e=t),n&&e!==!1&&this.queue(e||"fx",[]),this.each(function(){var t=!0,n=e!=null&&e+"queueHooks",s=v.timers,o=v._data(this);if(n)o[n]&&o[n].stop&&i(o[n]);else for(n in o)o[n]&&o[n].stop&&Wn.test(n)&&i(o[n]);for(n=s.length;n--;)s[n].elem===this&&(e==null||s[n].queue===e)&&(s[n].anim.stop(r),t=!1,s.splice(n,1));(t||!r)&&v.dequeue(this,e)})}}),v.each({slideDown:Zn("show"),slideUp:Zn("hide"),slideToggle:Zn("toggle"),fadeIn:{opacity:"show"},fadeOut:{opacity:"hide"},fadeToggle:{opacity:"toggle"}},function(e,t){v.fn[e]=function(e,n,r){return this.animate(t,e,n,r)}}),v.speed=function(e,t,n){var r=e&&typeof e=="object"?v.extend({},e):{complete:n||!n&&t||v.isFunction(e)&&e,duration:e,easing:n&&t||t&&!v.isFunction(t)&&t};r.duration=v.fx.off?0:typeof r.duration=="number"?r.duration:r.duration in v.fx.speeds?v.fx.speeds[r.duration]:v.fx.speeds._default;if(r.queue==null||r.queue===!0)r.queue="fx";return r.old=r.complete,r.complete=function(){v.isFunction(r.old)&&r.old.call(this),r.queue&&v.dequeue(this,r.queue)},r},v.easing={linear:function(e){return e},swing:function(e){return.5-Math.cos(e*Math.PI)/2}},v.timers=[],v.fx=Yn.prototype.init,v.fx.tick=function(){var e,n=v.timers,r=0;qn=v.now();for(;r<n.length;r++)e=n[r],!e()&&n[r]===e&&n.splice(r--,1);n.length||v.fx.stop(),qn=t},v.fx.timer=function(e){e()&&v.timers.push(e)&&!Rn&&(Rn=setInterval(v.fx.tick,v.fx.interval))},v.fx.interval=13,v.fx.stop=function(){clearInterval(Rn),Rn=null},v.fx.speeds={slow:600,fast:200,_default:400},v.fx.step={},v.expr&&v.expr.filters&&(v.expr.filters.animated=function(e){return v.grep(v.timers,function(t){return e===t.elem}).length});var er=/^(?:body|html)$/i;v.fn.offset=function(e){if(arguments.length)return e===t?this:this.each(function(t){v.offset.setOffset(this,e,t)});var n,r,i,s,o,u,a,f={top:0,left:0},l=this[0],c=l&&l.ownerDocument;if(!c)return;return(r=c.body)===l?v.offset.bodyOffset(l):(n=c.documentElement,v.contains(n,l)?(typeof l.getBoundingClientRect!="undefined"&&(f=l.getBoundingClientRect()),i=tr(c),s=n.clientTop||r.clientTop||0,o=n.clientLeft||r.clientLeft||0,u=i.pageYOffset||n.scrollTop,a=i.pageXOffset||n.scrollLeft,{top:f.top+u-s,left:f.left+a-o}):f)},v.offset={bodyOffset:function(e){var t=e.offsetTop,n=e.offsetLeft;return v.support.doesNotIncludeMarginInBodyOffset&&(t+=parseFloat(v.css(e,"marginTop"))||0,n+=parseFloat(v.css(e,"marginLeft"))||0),{top:t,left:n}},setOffset:function(e,t,n){var r=v.css(e,"position");r==="static"&&(e.style.position="relative");var i=v(e),s=i.offset(),o=v.css(e,"top"),u=v.css(e,"left"),a=(r==="absolute"||r==="fixed")&&v.inArray("auto",[o,u])>-1,f={},l={},c,h;a?(l=i.position(),c=l.top,h=l.left):(c=parseFloat(o)||0,h=parseFloat(u)||0),v.isFunction(t)&&(t=t.call(e,n,s)),t.top!=null&&(f.top=t.top-s.top+c),t.left!=null&&(f.left=t.left-s.left+h),"using"in t?t.using.call(e,f):i.css(f)}},v.fn.extend({position:function(){if(!this[0])return;var e=this[0],t=this.offsetParent(),n=this.offset(),r=er.test(t[0].nodeName)?{top:0,left:0}:t.offset();return n.top-=parseFloat(v.css(e,"marginTop"))||0,n.left-=parseFloat(v.css(e,"marginLeft"))||0,r.top+=parseFloat(v.css(t[0],"borderTopWidth"))||0,r.left+=parseFloat(v.css(t[0],"borderLeftWidth"))||0,{top:n.top-r.top,left:n.left-r.left}},offsetParent:function(){return this.map(function(){var e=this.offsetParent||i.body;while(e&&!er.test(e.nodeName)&&v.css(e,"position")==="static")e=e.offsetParent;return e||i.body})}}),v.each({scrollLeft:"pageXOffset",scrollTop:"pageYOffset"},function(e,n){var r=/Y/.test(n);v.fn[e]=function(i){return v.access(this,function(e,i,s){var o=tr(e);if(s===t)return o?n in o?o[n]:o.document.documentElement[i]:e[i];o?o.scrollTo(r?v(o).scrollLeft():s,r?s:v(o).scrollTop()):e[i]=s},e,i,arguments.length,null)}}),v.each({Height:"height",Width:"width"},function(e,n){v.each({padding:"inner"+e,content:n,"":"outer"+e},function(r,i){v.fn[i]=function(i,s){var o=arguments.length&&(r||typeof i!="boolean"),u=r||(i===!0||s===!0?"margin":"border");return v.access(this,function(n,r,i){var s;return v.isWindow(n)?n.document.documentElement["client"+e]:n.nodeType===9?(s=n.documentElement,Math.max(n.body["scroll"+e],s["scroll"+e],n.body["offset"+e],s["offset"+e],s["client"+e])):i===t?v.css(n,r,i,u):v.style(n,r,i,u)},n,o?i:t,o,null)}})}),e.jQuery=e.$=v,typeof define=="function"&&define.amd&&define.amd.jQuery&&define("jquery",[],function(){return v})})(window);

//Extend to allow :regex
//  Can't use regex in :contains
jQuery.extend( jQuery.expr[':'], { regex: function(a, i, m, r) { var r = new RegExp(m[3], 'ig'); return r.test(jQuery(a).text()); } });

/*
 * jQuery browser and OS detection plugin
 * http://www.stoimen.com/blog/2009/07/16/jquery-browser-and-os-detection-plugin/
 */
(function(){var a={init:function(){this.browser=this.searchString(this.dataBrowser)||"An unknown browser";this.version=this.searchVersion(navigator.userAgent)||this.searchVersion(navigator.appVersion)||"an unknown version";this.OS=this.searchString(this.dataOS)||"an unknown OS"},searchString:function(e){for(var b=0;b<e.length;b++){var c=e[b].string;var d=e[b].prop;this.versionSearchString=e[b].versionSearch||e[b].identity;if(c){if(c.indexOf(e[b].subString)!=-1){return e[b].identity}}else{if(d){return e[b].identity}}}},searchVersion:function(c){var b=c.indexOf(this.versionSearchString);if(b==-1){return}return parseFloat(c.substring(b+this.versionSearchString.length+1))},dataBrowser:[{string:navigator.userAgent,subString:"Chrome",identity:"Chrome"},{string:navigator.userAgent,subString:"OmniWeb",versionSearch:"OmniWeb/",identity:"OmniWeb"},{string:navigator.vendor,subString:"Apple",identity:"Safari",versionSearch:"Version"},{prop:window.opera,identity:"Opera"},{string:navigator.vendor,subString:"iCab",identity:"iCab"},{string:navigator.vendor,subString:"KDE",identity:"Konqueror"},{string:navigator.userAgent,subString:"Firefox",identity:"Firefox"},{string:navigator.vendor,subString:"Camino",identity:"Camino"},{string:navigator.userAgent,subString:"Netscape",identity:"Netscape"},{string:navigator.userAgent,subString:"MSIE",identity:"Explorer",versionSearch:"MSIE"},{string:navigator.userAgent,subString:"Gecko",identity:"Mozilla",versionSearch:"rv"},{string:navigator.userAgent,subString:"Mozilla",identity:"Netscape",versionSearch:"Mozilla"}],dataOS:[{string:navigator.platform,subString:"Win",identity:"Windows"},{string:navigator.platform,subString:"Mac",identity:"Mac"},{string:navigator.userAgent,subString:"iPhone",identity:"iPhone/iPod"},{string:navigator.platform,subString:"Linux",identity:"Linux"}]};a.init();window.$.client={os:a.OS,browser:a.browser}})();

//======================
//START JSON2
//======================
if(!this.JSON){JSON=function(){function f(n){return n<10?'0'+n:n;}
Date.prototype.toJSON=function(){return this.getUTCFullYear()+'-'+
f(this.getUTCMonth()+1)+'-'+
f(this.getUTCDate())+'T'+
f(this.getUTCHours())+':'+
f(this.getUTCMinutes())+':'+
f(this.getUTCSeconds())+'Z';};var escapeable=/["\\\x00-\x1f\x7f-\x9f]/g,gap,indent,meta={'\b':'\\b','\t':'\\t','\n':'\\n','\f':'\\f','\r':'\\r','"':'\\"','\\':'\\\\'},rep;function quote(string){return escapeable.test(string)?'"'+string.replace(escapeable,function(a){var c=meta[a];if(typeof c==='string'){return c;}
c=a.charCodeAt();return'\\u00'+Math.floor(c/16).toString(16)+
(c%16).toString(16);})+'"':'"'+string+'"';}
function str(key,holder){var i,k,v,length,mind=gap,partial,value=holder[key];if(value&&typeof value==='object'&&typeof value.toJSON==='function'){value=value.toJSON(key);}
if(typeof rep==='function'){value=rep.call(holder,key,value);}
switch(typeof value){case'string':return quote(value);case'number':return isFinite(value)?String(value):'null';case'boolean':case'null':return String(value);case'object':if(!value){return'null';}
gap+=indent;partial=[];if(typeof value.length==='number'&&!(value.propertyIsEnumerable('length'))){length=value.length;for(i=0;i<length;i+=1){partial[i]=str(i,value)||'null';}
v=partial.length===0?'[]':gap?'[\n'+gap+partial.join(',\n'+gap)+'\n'+mind+']':'['+partial.join(',')+']';gap=mind;return v;}
if(typeof rep==='object'){length=rep.length;for(i=0;i<length;i+=1){k=rep[i];if(typeof k==='string'){v=str(k,value,rep);if(v){partial.push(quote(k)+(gap?': ':':')+v);}}}}else{for(k in value){v=str(k,value,rep);if(v){partial.push(quote(k)+(gap?': ':':')+v);}}}
v=partial.length===0?'{}':gap?'{\n'+gap+partial.join(',\n'+gap)+'\n'+mind+'}':'{'+partial.join(',')+'}';gap=mind;return v;}}
return{stringify:function(value,replacer,space){var i;gap='';indent='';if(space){if(typeof space==='number'){for(i=0;i<space;i+=1){indent+=' ';}}else if(typeof space==='string'){indent=space;}}
if(!replacer){rep=function(key,value){if(!Object.hasOwnProperty.call(this,key)){return undefined;}
return value;};}else if(typeof replacer==='function'||(typeof replacer==='object'&&typeof replacer.length==='number')){rep=replacer;}else{throw new Error('JSON.stringify');}
return str('',{'':value});},parse:function(text,reviver){var j;function walk(holder,key){var k,v,value=holder[key];if(value&&typeof value==='object'){for(k in value){if(Object.hasOwnProperty.call(value,k)){v=walk(value,k);if(v!==undefined){value[k]=v;}else{delete value[k];}}}}
return reviver.call(holder,key,value);}
if(/^[\],:{}\s]*$/.test(text.replace(/\\["\\\/bfnrtu]/g,'@').replace(/"[^"\\\n\r]*"|true|false|null|-?\d+(?:\.\d*)?(?:[eE][+\-]?\d+)?/g,']').replace(/(?:^|:|,)(?:\s*\[)+/g,''))){j=eval('('+text+')');return typeof reviver==='function'?walk({'':j},''):j;}
throw new SyntaxError('JSON.parse');},quote:quote};}();}
//======================
//END JSON2
//======================

//Function to parse version numbers - maj.min.patch
//http://maymay.net/blog/2008/06/15/ridiculously-simple-javascript-version-string-to-object-parser/
function parseVersionString(str) {
    if (typeof(str) != 'string') { return false; }
    var x = str.split('.');
    // parse from string or default to 0 if can't parse
    var maj = parseInt(x[0]) || 0;
    var min = parseInt(x[1]) || 0;
    var pat = parseInt(x[2]) || 0;
    return {
        major: maj,
        minor: min,
        patch: pat
    }
}

//http://stackoverflow.com/questions/18082/validate-numbers-in-javascript-isnumeric
function isNumber(n) {
  return !isNaN(parseFloat(n)) && isFinite(n);
}

//=======================
//jQuery noConflict
//=======================
var $ljq = jQuery.noConflict();
if ($ljq.browser.webkit) {
	//GMlog("this is webkit!");
	isWebkit = true;
}
//$ljq.each($ljq.browser, function(i, val) {
//	GMlog(i+" : "+val);
//})

//GMlog($ljq.client.os);
//GMlog($ljq.client.browser);

//Store info in LocalStorage for Chrome
if (isWebkit) {
	if (!this.GM_getValue || (this.GM_getValue.toString && this.GM_getValue.toString().indexOf("not supported")>-1)) {
		this.GM_getValue=function (key,def) {
			return localStorage[key] || def;
		};
		this.GM_setValue=function (key,value) {
			return localStorage[key]=value;
		};
		this.GM_deleteValue=function (key) {
			return delete localStorage[key];
		};
	}
}

//gm_title for customized titles
//GM_deleteValue('gm_title');
var gm_title = GM_getValue('gm_title', '');
GMlog("gm_title = " +gm_title);
if (gm_title == '') {
	//alert("0");
	GMlog("gm_title reset to %T");
	GM_setValue('gm_title', '%T');
	gm_title = GM_getValue('gm_title', '');
}
//alert(gm_title);

//alert($ljq("#yt-feedback").css("display") + " - " + $ljq("#yt-feedback").css("visibility"));
//Check for playlist and raise or lower YouTube Download box
//alert($ljq("#quicklist").css('display')+'\n'+$ljq("#quicklist #quicklist-player-container").css('display'));
var quicklist = 0;
var ytfeedback = false;
function playlistbarstatus(clicked) {
//check for About the new look | Send feedback menu
if (!clicked) {
	try {
		if ($ljq("#yt-feedback").css("display") == "block") {
			ytfeedback = true;
			GMlog("yt-feedback");
		}		
	} catch(e) {}
}

//check for playlist
try {
	GMlog("Check playlist");
	if (!clicked) {
		GMlog("Check !clicked");
		try {
			quicklist = $ljq("#playlist-bar").attr("data-list-id").length;
		} catch(e) {
			GMlog("quicklist length failed!");
		}
		if (quicklist > 0) {
			GMlog("quicklist > 0");
			if ($ljq("#playlist-bar-toggle-button").attr("data-tooltip-text").indexOf("Show") !== -1) {
				quicklist = "65";
			} else {
				quicklist = "165";
			}
		} else {
			GMlog("quicklist < 0");
			if (!ytfeedback) {
				//no feedback
				quicklist = "5";
			} else {
				//yes feedback
				quicklist = "40";
			}
		}
	} else {
		GMlog("Check clicked");
		//data-tooltip show/hide
		//alert($ljq("#playlist-bar-toggle-button").attr("data-tooltip"));
		if ($ljq("#playlist-bar-toggle-button").attr("data-tooltip-text").indexOf("Show") !== -1) {
			//HIDE
			$ljq("#gmmnu").css({bottom:"65px"});
			GMlog("Hide playlist");
		} else {
			//SHOW
			$ljq("#gmmnu").css({bottom:"165px"});
			GMlog("Show playlist");
		}
	}
} catch(e) {}
}
playlistbarstatus();

//********************************
//Store images in base64 to save bandwidth!
//********************************
var icon_donate_paypal = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAEsAAAAwCAYAAAEurLd%2BAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAyJpVFh0WE1MOmNvbS5hZG9iZS54bXAAAAAAADw%2FeHBhY2tldCBiZWdpbj0i77u%2FIiBpZD0iVzVNME1wQ2VoaUh6cmVTek5UY3prYzlkIj8%2BIDx4OnhtcG1ldGEgeG1sbnM6eD0iYWRvYmU6bnM6bWV0YS8iIHg6eG1wdGs9IkFkb2JlIFhNUCBDb3JlIDUuMC1jMDYwIDYxLjEzNDc3NywgMjAxMC8wMi8xMi0xNzozMjowMCAgICAgICAgIj4gPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4gPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIgeG1sbnM6eG1wPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvIiB4bWxuczp4bXBNTT0iaHR0cDovL25zLmFkb2JlLmNvbS94YXAvMS4wL21tLyIgeG1sbnM6c3RSZWY9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9zVHlwZS9SZXNvdXJjZVJlZiMiIHhtcDpDcmVhdG9yVG9vbD0iQWRvYmUgUGhvdG9zaG9wIENTNSBNYWNpbnRvc2giIHhtcE1NOkluc3RhbmNlSUQ9InhtcC5paWQ6RTQ1MzYzMUU3Q0YxMTFFMDg0RkVCRkY5NjUwNzIyREMiIHhtcE1NOkRvY3VtZW50SUQ9InhtcC5kaWQ6RTQ1MzYzMUY3Q0YxMTFFMDg0RkVCRkY5NjUwNzIyREMiPiA8eG1wTU06RGVyaXZlZEZyb20gc3RSZWY6aW5zdGFuY2VJRD0ieG1wLmlpZDo0RjhCQ0M3QjdDRTYxMUUwODRGRUJGRjk2NTA3MjJEQyIgc3RSZWY6ZG9jdW1lbnRJRD0ieG1wLmRpZDo0RjhCQ0M3QzdDRTYxMUUwODRGRUJGRjk2NTA3MjJEQyIvPiA8L3JkZjpEZXNjcmlwdGlvbj4gPC9yZGY6UkRGPiA8L3g6eG1wbWV0YT4gPD94cGFja2V0IGVuZD0iciI%2FPsztcggAABBkSURBVHjaYvy%2FPDCQ4cvTbgZC4P8ss1v%2F%2F3z7jxf%2F%2B%2FWfBaz6z2eG%2F%2F%2F%2FoxjAyAg0BCjEyASk%2F7IwgBUyOi1nYPj5j8HaTJThzK2PDMXlrgzLb%2F5jmOvGzJCw4y%2FD%2FWR2Bsb%2Fs0xuMYTPVv375z8DMwsjA6P7QYb%2F2%2B0Y%2Fv5jYGBmhJj%2Bl5EZZCIjw9%2FvHyECv4Gu2GDA8PfnJwgf5g4mZgaAAGL8P9vsDiEPM4J8REgRy%2F%2Ffn4A%2BYmT4%2F%2B8%2Fhk%2FBbGYmBqb%2Fvz8z5Ew9BXTdNwZGth8MTE4rGEr3fWT4%2Feszg%2FWy9wz%2FfnxkYPzz5tx%2FoAqwD0Fh9w%2FqYqABDH9BbKAcC8OfT3DfIYO%2FSGyAAGL4PwcYVf%2F%2B%2FKcYAwELAzCQQe6nGDBBoxxkmGXhQYZ%2FwBA7c%2FMDw%2F%2F9wQznrr9jMFThZ3j9%2FhcDDyczUC0jg%2Bmy7wzfgd68k8HDcPHpXwZ9cSYGselfGF5l8TD8%2Bw01jAEY%2FKduvGf4v8OFgQGoUSViG8PdR18Z%2Fu92ZRCP3MvwcokNg8E6ToZn6X%2BBoczAoAA0oMH0HzDkGRiE2JkY%2Fn3%2FBEsRprf%2B%2Bk9SxeZyZg5mYFr6z%2FD3zz8ivAlKh8DowxVmf36TEmbMDAABWCl7l4SiMIw%2FR%2B9NMQ36kpYoGoMiyFzKhi4NDQ71HwT9BQ3u7Q3RJNVauQRCEA0N0dAgVGCzRkS1BPcqVN7bOff0npuomQ1mZzvnwMP78Xsexepd84b%2FdLrCl0ytlNzX%2BTYPkpbmFeU6%2BI%2BjSWGTXudiZB5agNq1%2B%2BE502PObe1kVk2PpqjyMsozHIn5JFXFGAczMggnj3Cae0C%2B8AKmCxQeiT36Lz5ZGE6bWM6WMLZnwnyz8VyukP0dGJnSV2dSGdilNiVHuFvHa4VjMdaL82sSS5whENJwmJrCQE8XgloEg0GJ5CiDQ7TGMxxpw4%2BteQaXU4SoyijcsLp5g%2Fx2HO6xAX0ui9nJCIaiQSzF%2BrGycYXEeAgB4ndngSMWFTDfHejU3vqFwEQfVaXmTmKMW7fST22qlPWoV9FancvIWg7F3Rnvrp6FrGdTLYKq5vCdpAgNKlG4vP7ZgNz9%2FjRE2f6RW6IVlrWZNYh9Cz7LbgMOqThzfhVrizNi5lMA5ssvpKkojuOf290mW1Eya8RcQesPakg5ZhFRKENq9CKISUQMLJMegnwpCKQXIeilHnqKwkE9hC8ZFCUqCdGD%2BKAMCoI0Q0RCE00377brPZ17r5tQQQUX8rzsPhx2zz2%2F8%2Ft8vkeStnZCbLTxJL4ot0yxP9Gh9nSC%2FuZqbAMYzrS6I0PC0EZGYbc2yo5h95O9sFXNcr5JpJ630yT7J4vcqj%2B0g%2BvnKqRwjL87uBIGra8yrKFR%2FircrfPic%2F1urkJ7X4b5rCBRtsDX7zlaT%2B%2BzXrxeShO0HpWWzncWM8XwWUZTs0SuDBHe6aG5bQhCW6SrV61%2BFiMtKJGnVB308z4ZQ431YmR0hnsaeTOlc%2FOIi7YaF5UPNSqSeYk8N5cHZTBwKxZ4Yrs2ceeEi9eTeToiKmcOl1mcEhL6P5Uyz3BKwleKPuAvoep8H7u3l6C9PEVT1xgEfYTLfUzOanK6XJyWJhDazPhMGuXkM%2Fx%2BD98G49R2Zy1T9H7SeT6u01EDl6oVQo%2FyeGVSOVAKn6Xtw9sE7QM5MypwLWrI%2FywcJXW9lKbnVNWgsWsUVgymk8dQCtNcOh%2Bmli0vBUrdTHxZpjMRRmgaty%2Fs4eL9j0QrtzJyL0p6PsNM2kNdUPC4IU%2FBcHNLZgnca2QVLOUUbh3Pyujkob5cxkrJ%2F6LthCqfhZmwZfppfrD%2Fl1z5h9E%2Ftki8I0WiKUj31b0YwilcyAz44sbC2mUi%2B8%2FEbqj2og8ctU%2BC7mBHC9XWSKGUTqjEqYUpliwRXlbmKN4f%2Fjv5rfue%2B4cAzJhrbBRVFMd%2Fsztb9iGVNn1tEUItWsBiUpAS1Gp9gFmjBCP9gCHEGNEoqF9UEuIHjDF%2BtBhqNCE2hrQxMRLpJxAQ5RErNoIutbWlGBJaHtp2uy3dLjsPz9zd7VIKEROMs8ndmcydyfnfe8495%2F8%2Fmt388FGMRBnu%2BQV0SqtriezwuQZSckzJh1tD0m%2FVT6qP0jSuAiUZTk%2FXExeBss2M%2BnNLWlAJ0LU7lblxGShLKbScFtSmyJFr%2B1r%2FLEizf9fXmtPe9eQ%2BULo1HVNMZnTNrxPZfBAtw9jyBOCuLbXcFtBvGtSRAZPPO5OTgJaUedlU4xdb09H1jli8c3SC56s0gkKR6qqL0LIxZWfcN3gpwd6fLzKr0M%2F88hAd3cPkR77CPr6OC%2F2jnJX5eaVBSmX%2BwuCEaF%2BTirKg4lVnhG95ZRFNJz10XDS5t8hDX8zi23MGJaKBGxb56PnTIimmFpd4VMR83XuF9vMGjfVBwqECYR6mwpIJ9LT7fuySOijMdNv6Kt54roqCJ%2FcQ%2B8vkg%2BYoW5uiUi1lr2WLX362Er%2BI4u2tPYx%2Bs5qTfXHqNn3Pl9vuo2uwWBnevcZHfBxqWpLCqSyCH45RGtSQdci8zbkX%2Few7ayBElnC%2B6XDPjL8NR6yn%2FajJ2L7njDLa0TvMyte%2FIzaS4tWGSt5uuJPTX6xk9%2FvLHZ%2FS3jUsIr4YxN0hob6Pv3kMf8jH2ifuYOCyPBbs7x5LsbQ1SUKMvlAN46%2Fl0RLxsmGBh3z5ZlgmOgdtKmdlwsfODiuXEjShsIdPDSllf%2BL0CLpcG19ZwMan5qI%2F2qZUfsWckKrkm5%2Bew0OLZqqFvfTRLyRFdPz66YP09Dv8HAr88EO%2Fxf1h%2BLheY8sRk5Zum4p8GBINc1lsz55p4XTUlpU4DN2cnjwVp8rTSY6m0EM6p3auUP03S4y91fw7%2BLycb6lj664%2B%2FhgYZ01tIYEZqF3dua%2Bf8rIAi%2BcF%2BCRqqZPUWCeAyu20opJda%2B32SszAoQaTymYvs%2BX%2BJzkQmuZhw90WU2is4z47E%2BjRziE1uXp5EXYiJRo2pZ5vXCWsRsCF1x1m%2Fwl5Z8ygqNCHYmSOUQmgzh3LZIEGn%2F2mIetiRdhptZiYjltk1BTbyBnhmTaPsr%2BwwKYpmo6vpcWGspMbV7nvnrlybA8%2Bogxd3XK5K5yHfaB%2BMm85gs%2BcuMKluFgXlz%2B2pJDbA%2FKZGD%2B%2BNpEuqcbUdkxbxMy2wXLXTHoyresmz%2FTpc7xo3kCPXqsxvCLPyte3Q9zgwHsLBYRx434QuebTzWV0IxtT%2F67MmAnZ5kMPqPWY8ZR7CrIZ%2B4%2FqZTqmmFL7%2Fv%2BCbGT5lItYgnKfbbmQuliGzkTMPaBS4%2FwtQLvWGhtVFYS%2Fe%2FfZQivbdwFbKDSlSh%2FEIkaBolBQ5KmYgEGCMeIDIqIIBuIfE4lCUDRA%2FUEEUwJBIJgAQR4iFhTBglQBK62FPgJtoVBa2n3ee5w5d3e7fUkwIanJ3rTZzd57zznzzcyZme%2BMIraMK4ZqzoZjaET7BhK%2BOmR9mkfBzfJrZvjakpCcF4HJG6wQYbC6PbFpowi7Z260uaMxaWFwulxmPy6Kv4KQFqX3HtKqN11qIPQFGGTa4UUgXw5fnaKNIvFpp9sDlXwYrB7A0oIOaaTR7IJhN%2BwhwfJ1AitsWXexLBEKlq9bsDhycmGpdB6DflOttPuZqSZ3e2Viez%2Bjd3f8mVyDWTGaC7zinjMfHtfFHTq6LHPh8mhoowLd0dcq%2BxisTPT49yy%2FZRk9WkKC1e6Gqp%2Fsm7e6BEXbyoAoi1FwevyoMFA8G328MGkQtryXh0gmM%2Fg2C8DvB%2Fq%2F6D3h040shQXktosAmUia0EPaieS8ZjWEIRRYecyFwlKP7N7gJbj9OrWajGFMNM%2FEQWYUjrczRyTnkI0rnQhLfjkwk2pVsPuiF4uOOo06mv73zYiAr%2FE2Lrh1efT52rODjfcIl8CRUrDw6VBHW0woq2zGzuJaIJ6S%2BzYfXpo%2BBGsXZkshth2uwfubLsDl1bDzSDXys2PxaKYDHxeV4fJ1F%2Bqa3JJAshIwAx02jBsRjw%2FmZCA6NgLLN5Ti2x9roZpUjM6JwwYaU2qRhFtc%2BDsOnqqTgo3KisGqBbk4WaehLwnHsj85UMXafKtc9c9XNSw5ZtTvh674sP43F%2BZkWrDujBcVt3U0OoVkz1h3%2FWxAVpyKN3MsGByjwkP3jlZ7Zf7Ehvlcugm5cWRNSQ7ohOiE3Fg%2FHkoQF78b6v7EKwQs0vb2Y9VwtXqlENGJkXhreiri4yJoIhcuVN%2BGi1dCM%2FWJsuKp7BgUn2%2BEmSxufsFADIi1o4nuf7KjAiWXmlDy5y3sPn4VlTsmYnCiDeX1TvJ6Ha0EtsfrlQTatJWncPBEnZyv4LEEfL0iB5vOavjrpsHAsVG%2F%2B4iKGAd9cQJljRp7nwQxxg7ZfHSy1oNWj8DkVAWpUSpcpNg95QRMjY6yWxoOVmnYO9XCTZzyO4%2FLDvB0CgFl13pop9BD86yO0ZBfdrbp%2BHJ%2FldFgRIA0k6WMfPloO%2FfDrkR%2BMGVUArYuz4GF1HejyYVzZI0Hfm1ACwPJz5qN99ndxg53yO678dkORPUxo7nFi%2FomD67U3cHCjRdRfLpBDj1%2FSgo2L8tGS7OGI1WGAnkItpD8nT65hwq%2FG7JrjklWsC5fwQM2DafrBa60CPxCmDPJJztUVeM51ntuHDkKgfLpOSHdmccamahgfIou22C7lseic1LaMRoqNhWb9lajgQQJEHRfLMrE9McTSFZdukwyuZaJBOY9q2hfDeatOS%2Bfi4g04bNXMzCJni0uacSC9Rfh5lWR%2Bt%2BZ8SCZhxfp%2Fe0YnGBHabMHHrLgnEU%2FkaULicbS2WlY8%2FowCeqZej5TULgFE3dI0BV5As9n0C2Poav4SALCYqy%2F5Bow64DhqrR8vJElMC0duNVK4B8hZXsM0CalCPqkraNCkWPwmp8hoCItPWROIWckXZJSPoX2tmn45ngdhwqpkkGJdrwyIQmR9F0Phhwy2VZ6njb%2BCtoMJf9K4DlJOxv312D1rsuorG0zpKKIM%2FWJBGT0txnUKQlUMMKB0nKjSZc7XFnN6xcPw8KZKRyWpEVsv6QSfopUeBZtITPTNCTTELqlXRB2G5XArG2h5zSTJNfZXfdUAodrFNDOII2be00y%2Bgk5xlcXCDy3SXoQE%2FEvptPae6IuRXvNrMje7oRhqfq4ZVamZ1Wy9xu3vfijqlW2AzI2acl2pCXZewzN3LZc2%2BDGd2dvotmpI29oXzycGomKq04039EkwMMH9cGAOBs0siBuql23qwZLCssN36K%2F7UszMXtiMjQKJGzMtO3gTIMRFRm4xEiBDIfokVlmq7npVHCiTsHVVhUPxegYESfALTWNbiMqJkYIpBNg566r0kUZLP5tSPS%2F5BysCWcT1O8%2FbJLd6CIhM1XPX2q9136Z%2F5IvqWSdbxf%2Bjc931cr0g0%2FKfvhoOMZkRdP0vZAiCoK1qskczIV0333N4BV%2F2rX1UD2uNzgxtyBe%2FrBiVn9kktVpnl5aagl0zuD9p8%2F3kYWXMYSmmTO6n%2FzvwKK5Pei1lwiNhgLdJ6XhKyTNCu0hFSLMZ93FDRUEyx1x393w%2FwtWAJugZRklUBdqIXyF4MJttpvHFtFe9aSw2KIRPgvrybrMis9d%2Fg%2BpgX0a2UfHIQAAAABJRU5ErkJggg%3D%3D';
var icon_donate_flattr = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAEsAAAAUCAYAAAGykfBeAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAyJpVFh0WE1MOmNvbS5hZG9iZS54bXAAAAAAADw%2FeHBhY2tldCBiZWdpbj0i77u%2FIiBpZD0iVzVNME1wQ2VoaUh6cmVTek5UY3prYzlkIj8%2BIDx4OnhtcG1ldGEgeG1sbnM6eD0iYWRvYmU6bnM6bWV0YS8iIHg6eG1wdGs9IkFkb2JlIFhNUCBDb3JlIDUuMC1jMDYwIDYxLjEzNDc3NywgMjAxMC8wMi8xMi0xNzozMjowMCAgICAgICAgIj4gPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4gPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIgeG1sbnM6eG1wPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvIiB4bWxuczp4bXBNTT0iaHR0cDovL25zLmFkb2JlLmNvbS94YXAvMS4wL21tLyIgeG1sbnM6c3RSZWY9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9zVHlwZS9SZXNvdXJjZVJlZiMiIHhtcDpDcmVhdG9yVG9vbD0iQWRvYmUgUGhvdG9zaG9wIENTNSBNYWNpbnRvc2giIHhtcE1NOkluc3RhbmNlSUQ9InhtcC5paWQ6NEY4QkNDNzk3Q0U2MTFFMDg0RkVCRkY5NjUwNzIyREMiIHhtcE1NOkRvY3VtZW50SUQ9InhtcC5kaWQ6NEY4QkNDN0E3Q0U2MTFFMDg0RkVCRkY5NjUwNzIyREMiPiA8eG1wTU06RGVyaXZlZEZyb20gc3RSZWY6aW5zdGFuY2VJRD0ieG1wLmlpZDo0RjhCQ0M3NzdDRTYxMUUwODRGRUJGRjk2NTA3MjJEQyIgc3RSZWY6ZG9jdW1lbnRJRD0ieG1wLmRpZDo0RjhCQ0M3ODdDRTYxMUUwODRGRUJGRjk2NTA3MjJEQyIvPiA8L3JkZjpEZXNjcmlwdGlvbj4gPC9yZGY6UkRGPiA8L3g6eG1wbWV0YT4gPD94cGFja2V0IGVuZD0iciI%2FPsFmLIcAAARaSURBVHjaYmQAgpo5Af8Z8IMLYEWT98X%2FB4HZW2vANIwPAyA%2BI0ghJz8bAyEAEECMRFjLwAAyctXRDrgVMKtRrGxdHUbQJIAAAlsnrsTPQA3A8uXjDwbOtwhPVoWshLOnb6hiyAxoA7Pb1oSjyIFARp8dw4yiQ2A5EAAIIJDLzgNpA0pd1ZKygZERFCDU8OLLex8ZmGCcHMcFDMhsGB%2BdzfFdFswG0ch6QIDp09vvDCAMAjA2KKxgfFC4gMIExn7x5SaYfebGQRQ9oLAnKsqRwb9%2F%2FxmYmBgxxL9%2F%2FMUAEEDgCAAmDQOGQQSA4X%2BBCVuqgIUtctgjA%2FS4wgaQ4wyXXjz6DZhAcfD5%2FXeGXz%2F%2FwEVTvJoxVN6%2F9xRrYkC2QJxdE6ccumdA5r179xEu9h%2BYCEBuALkF5CZwiv3%2Fj4Hhx5ffDMgJDpSwYBkOJNabvBvu4CV7W8GGw9TCLL397ByKGTDxllVhcHNgAGSekoAJXPzzu%2B9gN4DcAsvkAb9%2B%2F13PDSwaWViZaZJmcCVydPDn91%2BGr8CEz8bKHAgQQGCHAcXWD6K0H8jCzs26XkCca9C46MPLb%2BuZuIio1egJQO5hQhcE5Y4A9QKcWRxf9idUhBAqbuBFPbpAkmU9AyMjI8OU%2FQkY2RubBdjqF2yOgdU32NRiOAqULb99%2FgUXkJEyYJCW1EfR1LQ2kKF4riuKRpijkR2Pj41eDqKb9%2FfPP7A7QO5hYgDm1j%2B%2F%2FmItq2D8uuD1wOz6E6XMuXL%2BHphm%2FiiFUanC5GHsD2%2B%2BwS2HVa6g8gpZH8h8kDtA7mEiJvGBCk1QKwYZgPggcVjrh5B6dL1422%2F1S4L%2Bs7Gz0Cw3%2FQdBYHXCxMhIlHpQtcP098c%2FlDpwIAHIHSD3AAQYvETn5GVj4OBlZRgFqODH598M3yEFQSAoPa0XlOJmYONgGQ0ZLAAULqBE9P7Z1%2FVEhxCoEvry%2BRvD37%2F%2F4GKKStLg5huMjQ9gU0esXnLNIMZ8UtxAVGBx%2FpQDFu9NGOKgegRURMPYMIuRAb8AD4OQED9cHayeATkOmxgoUj5%2B%2BIISIcjNV1xmgOpEmD6YneiRjWwuun7k%2BhFnYH3%2F%2BouB9xcHAyuwhMdVACd7QAJqzrZahuvPT2D1CLJnsAUquhpcYqA6F5defGZg04fPXGIDCFQzgepnUDixgFsQX34B2%2FGI7MUnzIlVI3prBN0yUGBi67khA1AKQI91bGL4PIJNbvWxToaXP6%2FjbLaB5EOtyuH82VtrGH6z%2FIXzf377DS5iQOHx%2Fx%2BW2hAojjUbIvfckAGsh4hLT05IMwMh82AxjGwWNjFcbkBu5yKrv%2FnkLLh1hk0%2FxG3lKGakereA9YNaeyAgJydJeEyuoNfjPz%2Bw30XLhhatAbghB0oNjMQ35khtZ318%2BY2BiYef48LPL38YRgFuAAofUDgxQgeaQQ3TetAQEmi0ZsilrP8g%2FB%2BYsICQiXrmAgMIRF0A4saWlA0bAC0Ivj8IYxHPAAAAAElFTkSuQmCC';
var gb_overlay = 'data:image/png;base64,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%3D';
var gb_blank = 'data:image/gif;base64,R0lGODlhAQABAJH/AP///wAAAMDAwAAAACH5BAEAAAIALAAAAAABAAEAAAICVAEAOw%3D%3D';
var gb_close = 'data:image/gif;base64,R0lGODlhUQAPAKIAAIiIiP////T09Nvb2wAAAAAAAAAAAAAAACH5BAAAAAAALAAAAABRAA8AAAONCLrc/jDKSau9OOvNOxhgKA5O4HHmNghs25JNesaTnK2uCzP2bPUXHKE1FJACSIUsuUAugUulkjl9Nq8OnGBYNPZSYClPfDWFAee02Ix+aLe677r5pErZ7bxaf883tFwvcnxjfiV+e4llEEJELEdMVlU8dn1ziFR2eFk5Oj6fKp0voKRBIyOlqaqrrA8JADs%3D';
var update_grey = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADwAAABFCAYAAAAW0YV7AAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAADTNpVFh0WE1MOmNvbS5hZG9iZS54bXAAAAAAADw/eHBhY2tldCBiZWdpbj0i77u/IiBpZD0iVzVNME1wQ2VoaUh6cmVTek5UY3prYzlkIj8+Cjx4OnhtcG1ldGEgeG1sbnM6eD0iYWRvYmU6bnM6bWV0YS8iIHg6eG1wdGs9IkFkb2JlIFhNUCBDb3JlIDQuMi4yLWMwNjMgNTMuMzUyNjI0LCAyMDA4LzA3LzMwLTE4OjA1OjQxICAgICAgICAiPgogPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4KICA8cmRmOkRlc2NyaXB0aW9uIHJkZjphYm91dD0iIgogICAgeG1sbnM6ZGM9Imh0dHA6Ly9wdXJsLm9yZy9kYy9lbGVtZW50cy8xLjEvIgogICAgeG1sbnM6eG1wUmlnaHRzPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvcmlnaHRzLyIKICAgIHhtbG5zOnBob3Rvc2hvcD0iaHR0cDovL25zLmFkb2JlLmNvbS9waG90b3Nob3AvMS4wLyIKICAgIHhtbG5zOklwdGM0eG1wQ29yZT0iaHR0cDovL2lwdGMub3JnL3N0ZC9JcHRjNHhtcENvcmUvMS4wL3htbG5zLyIKICAgeG1wUmlnaHRzOldlYlN0YXRlbWVudD0iIgogICBwaG90b3Nob3A6QXV0aG9yc1Bvc2l0aW9uPSIiPgogICA8ZGM6cmlnaHRzPgogICAgPHJkZjpBbHQ+CiAgICAgPHJkZjpsaSB4bWw6bGFuZz0ieC1kZWZhdWx0Ii8+CiAgICA8L3JkZjpBbHQ+CiAgIDwvZGM6cmlnaHRzPgogICA8ZGM6Y3JlYXRvcj4KICAgIDxyZGY6U2VxPgogICAgIDxyZGY6bGkvPgogICAgPC9yZGY6U2VxPgogICA8L2RjOmNyZWF0b3I+CiAgIDxkYzp0aXRsZT4KICAgIDxyZGY6QWx0PgogICAgIDxyZGY6bGkgeG1sOmxhbmc9IngtZGVmYXVsdCI+YmV0X29uZTwvcmRmOmxpPgogICAgPC9yZGY6QWx0PgogICA8L2RjOnRpdGxlPgogICA8eG1wUmlnaHRzOlVzYWdlVGVybXM+CiAgICA8cmRmOkFsdD4KICAgICA8cmRmOmxpIHhtbDpsYW5nPSJ4LWRlZmF1bHQiLz4KICAgIDwvcmRmOkFsdD4KICAgPC94bXBSaWdodHM6VXNhZ2VUZXJtcz4KICAgPElwdGM0eG1wQ29yZTpDcmVhdG9yQ29udGFjdEluZm8KICAgIElwdGM0eG1wQ29yZTpDaUFkckV4dGFkcj0iIgogICAgSXB0YzR4bXBDb3JlOkNpQWRyQ2l0eT0iIgogICAgSXB0YzR4bXBDb3JlOkNpQWRyUmVnaW9uPSIiCiAgICBJcHRjNHhtcENvcmU6Q2lBZHJQY29kZT0iIgogICAgSXB0YzR4bXBDb3JlOkNpQWRyQ3RyeT0iIgogICAgSXB0YzR4bXBDb3JlOkNpVGVsV29yaz0iIgogICAgSXB0YzR4bXBDb3JlOkNpRW1haWxXb3JrPSIiCiAgICBJcHRjNHhtcENvcmU6Q2lVcmxXb3JrPSIiLz4KICA8L3JkZjpEZXNjcmlwdGlvbj4KIDwvcmRmOlJERj4KPC94OnhtcG1ldGE+CiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAKPD94cGFja2V0IGVuZD0idyI/Po1hJ38AABDfSURBVHja7FtpcBVVFu738rKQBEkCBBJIMLIJyCJbIWSGZRgEZIITgRhlEcYygo4MQgrHH4q4jFIloNRYKIwbClMChSIqBZZJaRWlIpBIICAQkCQYlkBCAknI0vN9PX1e3TT93usnxGWcrrrV/bpv973fWb5zzu1+Ll3Xtd/S5tZ+Y1uLAy4t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%3D%3D';
var update_green = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADwAAABFCAYAAAAW0YV7AAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAADTNpVFh0WE1MOmNvbS5hZG9iZS54bXAAAAAAADw/eHBhY2tldCBiZWdpbj0i77u/IiBpZD0iVzVNME1wQ2VoaUh6cmVTek5UY3prYzlkIj8+Cjx4OnhtcG1ldGEgeG1sbnM6eD0iYWRvYmU6bnM6bWV0YS8iIHg6eG1wdGs9IkFkb2JlIFhNUCBDb3JlIDQuMi4yLWMwNjMgNTMuMzUyNjI0LCAyMDA4LzA3LzMwLTE4OjA1OjQxICAgICAgICAiPgogPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4KICA8cmRmOkRlc2NyaXB0aW9uIHJkZjphYm91dD0iIgogICAgeG1sbnM6ZGM9Imh0dHA6Ly9wdXJsLm9yZy9kYy9lbGVtZW50cy8xLjEvIgogICAgeG1sbnM6eG1wUmlnaHRzPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvcmlnaHRzLyIKICAgIHhtbG5zOnBob3Rvc2hvcD0iaHR0cDovL25zLmFkb2JlLmNvbS9waG90b3Nob3AvMS4wLyIKICAgIHhtbG5zOklwdGM0eG1wQ29yZT0iaHR0cDovL2lwdGMub3JnL3N0ZC9JcHRjNHhtcENvcmUvMS4wL3htbG5zLyIKICAgeG1wUmlnaHRzOldlYlN0YXRlbWVudD0iIgogICBwaG90b3Nob3A6QXV0aG9yc1Bvc2l0aW9uPSIiPgogICA8ZGM6cmlnaHRzPgogICAgPHJkZjpBbHQ+CiAgICAgPHJkZjpsaSB4bWw6bGFuZz0ieC1kZWZhdWx0Ii8+CiAgICA8L3JkZjpBbHQ+CiAgIDwvZGM6cmlnaHRzPgogICA8ZGM6Y3JlYXRvcj4KICAgIDxyZGY6U2VxPgogICAgIDxyZGY6bGkvPgogICAgPC9yZGY6U2VxPgogICA8L2RjOmNyZWF0b3I+CiAgIDxkYzp0aXRsZT4KICAgIDxyZGY6QWx0PgogICAgIDxyZGY6bGkgeG1sOmxhbmc9IngtZGVmYXVsdCI+YmV0X29uZTwvcmRmOmxpPgogICAgPC9yZGY6QWx0PgogICA8L2RjOnRpdGxlPgogICA8eG1wUmlnaHRzOlVzYWdlVGVybXM+CiAgICA8cmRmOkFsdD4KICAgICA8cmRmOmxpIHhtbDpsYW5nPSJ4LWRlZmF1bHQiLz4KICAgIDwvcmRmOkFsdD4KICAgPC94bXBSaWdodHM6VXNhZ2VUZXJtcz4KICAgPElwdGM0eG1wQ29yZTpDcmVhdG9yQ29udGFjdEluZm8KICAgIElwdGM0eG1wQ29yZTpDaUFkckV4dGFkcj0iIgogICAgSXB0YzR4bXBDb3JlOkNpQWRyQ2l0eT0iIgogICAgSXB0YzR4bXBDb3JlOkNpQWRyUmVnaW9uPSIiCiAgICBJcHRjNHhtcENvcmU6Q2lBZHJQY29kZT0iIgogICAgSXB0YzR4bXBDb3JlOkNpQWRyQ3RyeT0iIgogICAgSXB0YzR4bXBDb3JlOkNpVGVsV29yaz0iIgogICAgSXB0YzR4bXBDb3JlOkNpRW1haWxXb3JrPSIiCiAgICBJcHRjNHhtcENvcmU6Q2lVcmxXb3JrPSIiLz4KICA8L3JkZjpEZXNjcmlwdGlvbj4KIDwvcmRmOlJERj4KPC94OnhtcG1ldGE+CiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAKPD94cGFja2V0IGVuZD0idyI/Po1hJ38AABdpSURBVHja7FsJeFXVtV5nulPmm5kkkEAI0YRAEAoIVLCAShUfTqjoU6RabdXSgkVr66PVV6u1avVRqiDCp8UWan32UZ+VKtjBgSlhCDKGkIRMZL6598z7dO19zh0yMImp9vXd79vf3meffc5Z/xr+tfY5CWdZFvwr/Xj4F/sNOuBDJxvj9zTUeM9m7dG25vjajra4QRWIuvRgtR+9ve6qoQ+NrfUvLTv4xLsbZ5xu7cObXppd8INJ1fkPT6x++eN3Lh0smQYNbH1nmyvr4Rmf7K951tp77FEr47ujAgh6ykBrH9y0elbmsok9B+t/am2vusoa/sPLdrSHgsI/FeAVf31z2txf3IjDdmxtVuXRxyzv4pL6Px6oGBa7bmPln0dlPTS97WDdy3i4zbKMl60ZTxWTtdu2XjQYcg1aDL9z4IMrvnrBeBzpYOkdMGb4Alh33SU5N697YF1zoEuia+o7W72L1v/olVXXXekvyi0EM9QKwKXDZUWZ3HuHts38wpDW/ubjqTet/eG3lr+1Zj4SUkrf84qhw7HWmqlThpcCEBmIpYGpyXD9lIUwdyS55O6NTy+h6xb9+slHFpaPnHDluAlghLpxnY764WBq/gioatj1ZdMi/Z69o+5w2gP//cKC21998r5PmmuTzlV28VwvCGoqd+1L31k/Pbt7dlNrBsxbuaauIK1s9UNX3PXLrxSNbaFr9jRU+y0jVFyYlg2WqaIrmdgM4HQPPDNvIUx6bsWy+ausptauI4t/fMudCDYEJjEwvgwwDAJF6bkQUv5UgoztK/Cnh+g9t9ceyn1y86v37K79y8JZI6xsEjwO1728+5Jt31lzfZzLbQ0a4N0N1fGy3FC28tZ1eHURnOzclrdy67of3rr6rjvnlM378WNX37OySw2lZScnpbq9PtBVBS2HYMBAKyuQlFAM/zHzS8k3rfrNy28vXgw+twQhOQgcoFJwDTEtSPYlg98dGnK0rSU50eNR7v71U9/efewvD9w6LiPzuSsugeyURGhtfw8uXl1d3hYKSQhYGzTAmQkpaqIvPdgpAyQnaJCeXAqP/NtPYOGU7bkPvbHiF1Ofuf3yVE/alolDs1i4mARjGMFwCJoC0lQDZo+aBI/MDcG0ggIGlikEvYAqxUI39kg+yE9OEJ9655Vrajprrr44S535p7suhbyUOAhpGgRlA9pCblwr9ggcZwxqDBf4szRJ8DQ1dzcyAQ29B3Q9AHmpo+HVu56BJy6fNveT+o+f8XsTmItSMBbGpomNoNvqhgJuKRkemHGZHds4R+dp/NpjHTiOh+R4HnYc/dvzT87Km7n6hkvBH+eBjpAMqqGBRTQ40d0NOf6RzRkJCWRQY5j+vK7EIwdbaqeMyrmIWY3Gp4qkhAjh2gnXwVB/NtS2daIyVAaGKgYcC1NLEmwcuFBw3bGqEYlz9Ano0XS4OC+Xu2N8KZRm+qAdY9xiHmA3TrDgSBt6BsQdlHhhcEmL5wBGZRburqg7CHPLTds6juAUTAiFK88rg9HZQZC1oC2k1RsUBc45x8S5LlYhIdOEOcUT8JhAhyyze1iRNQTHClQ09sCY3JLKf0hamjqi/IOPao9gygnYLkvJBgWi8UqPZS0EmklsZZi2q9pNj7iwQRmZ2G7OzuGxadI5+16KgffRqcubYJj2GoPQZxAIqV1Q0QTmrKLxH31maWlv47GU3fVHivfUH0h2u33G1OFjuiRB6E70xDWNzyve/fgfuw83d9aPTI7LAN00HAs4TOu4ccSizjxEjo2I1cLnw+vteWdtZI3jztgL6MHV7SehVc3cd0HW0AMfHj+Youh6JpJZ0l+P7UkipilMHFbSWTokb19Rek7grADvrDuUN+P5hZunDM0Y5RJTWGzurv8Q2rs79TZZaZXEuKOH6+vj/nf/drht4pWgWlEwJAwo1o0hZg6ioCNKisybvWMazBjg9pxLNGHzkTY42sKlXL/6vr8ElKaCDK/pT/ByLstUkBsI7DhsQFWrf++mb7wwe3xeUdMZAb93ZM9XZhd6R/32rrV45MGGaQ4Z1dCCUkANZNd3NGRX1lejywUwTXQj+ZgRoMySxIjkXsvqDdCKiVWrF9hYC5MBAVPFB5Qe8IhJsOKq+KFFqdbQzPg88EkEBN4hNI6Am2+DBRtqRr+87b3JCPiNMwIempK+fU1TSFeUFkmShmAqkR3BOEwpPijKHAElQ0aAosusASUSJ99GwfVl3wHAR+aiwCOKgIHCgTYR5pXmgoDpVzV0FvuygasdZQkIuEXH0rbHH/pe2ZTdZ0Va80ZPqQJx2CvPvvcixgxhOZLlSSwTDVNDy4agSw6AqitMCNO0yYqEScmKac6xESEuwyYip7ebwQS2502HnMJjgxEVPW8462jx0a1orGbXkdF19lx7vYuXYdX2dijInLXqKyNHV58VYJcgwos3fv+RJ7a+37q/diu4BSnCwGYMiPDYcvowWAbAjGFfxsxmhGmNyLw9ZgBNM0YhsQqy53WHwaONsjeCRWWzYzwvcibsbW6HDQdS639y5dceO6e0NG14yYnbJt++5N/XP41FRSPmXy5SCYUtF7asGQYfAzwMqL9F7Z6g1SJzVgzwGMuGrW2nJTOSoow+a3SaAtHxKad8d3MP3Dv9nvsn5BW2DoSLO91bS5oHy59a+NLMYe47fn7tMuhSNebWNhubrGS0+rCz5cyTmJRjnyPR9bHx6TQmshWuxKIERvqxe5T1o8RGwCvJ8J9bGqHFvOznW+796WLqpecM2N4OajBs+VVvf2da2WX3T1tAt4csnpmQlKh6gepNWv1Ax+TVWMCkV86NUQT0ZvZwGiMxCqCE6eIVWL+nHV4/lP/G3gfXXpMWl3hKPMLy5csHPNHS0ym+Xvn++DUfvDm/qul48baa6oIL0rwwPDWXxQ0Js3JMSiLglJoOCOLMmb3m7GY6oMzYuQHWEUdxhESvC5+jXsODDh/WBWHldh7TVXxtdXtrKnq4Hu/xNid5fNYZLbz54M6ilX/ecFNV48FrMuO9ZVMLimBGYQnkJKWCJFDXkaIAw+5MjEiFFbWaEePuUYtRwS2wBYaI4LFW7RsKMZ4Rm8ZiQiCkmVjKmnAioMLOhhDsa+atdiW1YvKIaa8vnDTntTkXTjjWD3DliSNpD73x/PKatgO3XVs2MX7emCmApRnwuFtQcUun4bbMIFqEqKJxG7WCRfq4dxhgzFzEfUlfcDGgiBmpz1mOt4jjQTp7Bgup8HmUnxYbtMISeGycyRi9rsuALdUa/LXG1Tksa/Ka9bc98mi+P6szArjsiVteK083b3xq3jJI9CZCUA1hnlNYQRGbV62I2/Wxclhw0jd2zT6AzAHPR0koCtAkCjYZ82wIFU3fjakRAqRVl90423K0x3005xyLvIVlKEEcBFbvCIIUf+2Lf7jr8a9HqCykBQsmD8vBjXYytPZ0ozXVCBBCYgGb0XGs5Yi91bNILIj+gGMtTCJERZjoLFWZQVDNTqzuunCMQC2NWdFBFQEY7cMhSZz9sj2vmRyEcNeahJXxmGwR/nC8YZiKO7AI4Idn3/3Q/RsefD1kQMptExfgdR7c5qmRGB3IwiQmRkkfdyZ94refpak7chSkhS4YwhTYipVbGwra47wYAGYxWipw0AfbwAkHYpcxC+PO6p3DJmyoSm16/qZFy72SqzdpvbZra9mSjY++MDYbJn1/9nwoTC9BLekohBwBHXXbWDCGw6JRsOSULk2YxQx0V0VvR5drwr4DLakyd+RYLcR9+vfOeKlbtKAdS/z1uzWo6SrYuva2H3398uKLDg3I0ie62jzf3PCzpX87/MelN5ePSrp1wizITBiCBboBihaKIakYy4LpkFNUCWYvt7ZjjrquanQjyEboUZpQkQE2z0Bynx4kNanIA7NoD0bAlmod3q+Jb7us9OrHH5/7tZ/nJacbZyw8Nu3/uPAHbz7/QEA5Pm9WYV76rFGjYUR6gVMW6lErkv5504rJk/RH2V3WWnFrV49gWxg/cIxgPpsPH1RXJ4M8TUdYS/uaslLGbFw2+5an51wwoeacKq3mnk6Y+vQ9qzWlddFjc2ZB2ZAcJBTDqbBIjCvHuDYjIIuVe4oRQJCN0C3XM7el13CccF4uO1DkCqi3T1oFWF+BW9v0sc9+tHTFt72idG6lZW1HS8KsFfevujDVO/97M2eDR+KxrJR7xyoxHKYljEhpfOpGCHrUk7h9rGduqxPZJh6O/wxh9vNo3NFhptE5eLXSgOT4qWtXLVj2jeKMPPmsAL/1yccZi15Z/ttrSwqnfWPadMbWqqFGrGo5VmTHhG7Eg6iMNgR5Ai1KSaibfamj1uS4wYI5wAs6tLSAj3u9SoWqtrx33r3vufnlOYWdpwW8t/GY/0s/Xfj24qlfmnDHxEtwox3CfOhsA9GVDdwtqTSNaF1oSRqXLaxX0X1tl+XPm2nP6+sgPtaDyfatwxrsasrd+u69z80dmzMiMCDgxu529/T/uvf3F2V0z75z4jTokAMMoI4ko+pBFpMK7jlVowdLTdl5bwWOJfnPCeLAPx+G8KaDKhzqHPnGvgfXXu/3JZj9AN+94dkHth544cnbyxOQ3g37i7ntvE7pZmdKLlLCfZEg9mduL1r6pV0yXDzy1nvW3fzdX/Z7iXekta48NwHLcMQpCQICtscMthW9EZv7Avy1E2/boJdsfLh35MyKJ3D4ZN2YAd9a3jn5q08v/k3F6EOtraXpCQDpcRakeCWIR/cQBQuVIGK2EdjYLX6+oCmooEZBcfZHPfb6h4MerEo7sD4+GaJplcdiKb1y1S03rDglaR3vaHFtOVgx7v1je8epulKyq74qV+RIhiTy8c3dnamG2Zl9Q4mFMcKKxM/VugGVg41VHMS70+qTfXEdBrF6VMNqHj1kVH16nL9q+shxO6eNKKnMTUrTz/oVj0EIi+WQrsGYJxe9Xph88JpxuPvQzM/fpWnu/VudBkSYtm7LN5+5nbk4dVtB+PQf00SeZ/H8q51/urgjuP/q0kwJQgYHhvX5NyrHuGwXVDV+cPOWwxVlVM7TgT3rr4f0Df9TW361dEKOIFAiMMgXo+kEWGFTmmFIj76zdslAfwTzqQC/e6iyqKX7yJwCv4S7Jvp++ovTqDyj0jywq37bNTtqD+d+JoB/+eHvr89L0t30ZTzTrtW70cChIa1b/c99Fo1akt6fEvJA5ySsJ3MS1fiVH7x53XkDDmoqX3li31xqXcWMPog4LyRMJLQj7RbsaOAHBWy4bT/BQ02n/YWBPpc+P3xOxYP8FBE+PFYxV9ZV7rz+5GH3iaNZncGGC1KHiSzvUiY08QGtmOuOtWvQ2ONRND3paGpCz4WoaG4w2Bs3atCKlWxNW9IBjm8fOSzZkPL9LkjyCGyzQOXKiBNhd2PDmMbuTv/w1My2Tw040ePTZc3S9rXobGvYjgm9SxbMZM+QfVOHT9j0+PjZ6zcf2F72xr41r+nEx1x+oCKBqp1Yp8+rsRVd3+sFK8Q/fPn9y4ampNf/esfmmz4+vuuKkN5yYbKPCGlxWBES6o2S7BJ447wsXJqdf/LRr37rprUf/s+t4/NLOjPiUyunF5bvmJx/4SfpCUksob97cOdsF29FYrivsIZp71UT3dYpAXXgzjXODRGL9d7S0fAh4OHFITePu3QTtsra9pZHPjq+/4KtR3dPaAm0jd1/4nDCs9fc/OKQpLSu8/4rngdnLti8dMaNmyM5zgSfoir+ru5uT4ILh7qWx2KK9AfsQnesbDRxSynAlKH9zzMhEGRFE0Cyx4Sx2QIjor6A6fYlpMhDTVnP7tFCngxPonJD2fSjN5RPpx+9Lfrp9Ew5+IyATZMIJjFdpmG6sdpyBw3dQ0ziNk3TTQhxmdh4HXSJWHiej+THSOyhuSqaVJCMgmMGdKd0a93JLpyLtSCriZF1XFZyu6r4glUttXlFaW7QTavXexz6bUHTjbhgMJQX0mVJETQtEApqPM+rCBSbqGo8pwqioAp4LPC8gTnaOiuWRkCiqmrxaMUURVHSsE8LyaF0VVHT8ThDUdUMWVEyZVnJDMqhHDfhPTLmDN1pBtjblYomBSStsG7dFUvWj04pPl7TrTHB9Zi11HLHOnWYlFF2dN3lS14LBoc27W1RwOLt+0TuSfOuYbhRjpyQomTLspyJsmQ6smSg9dOpnExeRUnF+URdN6ihuFMCJhbhNU33IthETdOScJyMPTY9BZtf0TS/ig1vlqqqapqqKulySM5K9SYJuslH0pWOJtxer0CiWdzw9NRFWxN4b8YlmaXtTV0CaH3yuIxM1hpwWV/OKulME+NSn51651aXMqJlZ4MCpvM9gV6j4C4tmZOkYEjOZopXWaNypKKMftZULQUblTeJyq9qKjYtDj1R7OfSqAle102voetedGMvxoQPNeoxDeIzTMOLVsdmeAyD9oQd47yHgClleVOIouNWDIH2oHsewpidkjLp+JLyeXslENNPtDd7y5LyxGwhr6cpWBef6pMYq9K3jY0BA/JdIwLF8Vm++vbmohTJK/9s8h27flLxu9EfHd+VU5wlgkei1pUgJy7NCiqhdBMsXRB4Gd1YJgJxEZG4REIkIggifUWNWHAzLwgoDvbAU2ySJOF6QRfD/+hhGIaEgFwYmxijxE0MjF2TeBA8jVkPbYZBbEVQ8AYD7NVM3VuUlCWXJhTX76zel5sopCp3Dp95+Kr8CS2arqd1GbKP4LVuEKXL08v1l5prIdFrpyhq4RMdHNyXM85UFS1PswxdEVTFI7pCj4y74cjvqvMCG+u2FgSNDve41PLjeXGphqwqibzAo25FFg8xLmtFmiiiHTgicoiA43CEAxQc490Uo9tEqg2LaYP1qCLLPg73zhhTIrFwE8HmRVSIJPCC8OPJC7fuaTmWm+vzG1m+FCughBJ004gzmUdQxWi+if4Rnrea8qA2UA+5SRLUdGgwnBsOZYl5cT1KkN5GphZRLI3DjQA3v3Baw5SckqZmuZMfkzn8BOVBdE9KRjpKQy0pOvKF+4is0Ft2ZC+7F22m5CyOagKTHccR+uqR2MeR3gzPYT1tEh571BrheNQaQcMT0+f2qFNyS+oUXYtTdDWOo+xJnRafhExMUM2GR3Ird+TPkB89/Bt/t9wDZjARlhbP6HB73LhvJ9RNVZ5nLKvg9Yps6SQ3ITUwwp8dMjlLpdxF/z4azzE5ODa25YtpJNr48DHFh2JzViSG8SE6ujM+kKfOj9qgCrdsqwuC/RnPgt7uY9+EPRidR6duz4u86eJcKnoVTWEuUzJpmNDGLHJRzij++9KNJytaj8RflF8UKE0v6NFormOC8zrPc9h4DeVhKYfDFIN8qKBYCsagIoiiQuOXKoXGMPZs3KvhdTxTXrTR1MXAx77xQDflDaRyjOcwSdGY9bJYpjHMxnY8O81NnDgn9hq3aYMLN4k2CtQBzNzNxYtYbAgYwyanEZbECLWaYwUdxwYKyYDb4GlPLc9jfrXzLI1jqgA+ClQWbbCoFKoEqgxekURRFkWJjo1TvuKheRjZGEnKoGC8DIgDGgnIbdCekhsFZYOkgGNBugiNKzq2ECjBuMeUx2KKpUKndLI/cFPWscJhE/YYnhYONuiI1RG0JtCeAedt4E5vg+YjVqbgReYNghZbgJz2nRYKLiBIF4J329Yz7UrLtqhrIGtGLUqJBBMGSxGAJEeEGFbl+nwRw8lw3CG/2tbGOKWW5mzgHKeHrS44lkfgGrW40yth4KLIwmHASos7m3+npWsIZWZkZAqIKsG2rgPWokAtCS0qOpa1+97MSUHyFuOB2OqHcwCzGoOgoAw4/TcXnpISI0bWh92cWjxibcfdKeHppwJ5zoAHUgAFgiB5akVayTCQaFXmwvY5wS4AHMCM9Cy28ekL2C6VmWs7YCPWpiBtF7cBhy1uOrUyORPAzwTw6RTh3JOzrQqR/Af2mHM4nuvzpxn2CSd9cDSuOQinE5YRzhXYPwTwP8Pv//9D/P/67+8CDABSBR8PIXCdSAAAAABJRU5ErkJggg%3D%3D';

//Led Icon Set
var icon_about = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8%2F9hAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAlBJREFUeNpi%2FP%2F%2FPwMy8KvcJwikEoHYGIgNgZgJiC9A8ZxN7U5vkNUzIhvgU7angYOdpVhXXYxVXISbWVSYm4kRqOb1u29%2FX7%2F99vfijZe%2Fv%2F%2F4PWFLl0sthgFexTt7FGQEspxsVDhefv7L%2BOHbX4ZP3%2F8ygGT5OJkZ%2BIFYio%2Fl%2F6GT937cvv92xrZe9yK4Ae7522qV5YUqTU2VOS88%2Bsbw%2By%2FE0GvPn4JpLUlpMM0CdI%2B%2BHBfDxQv3v9%2B8%2B6Z350SvWkanrI1SHOysN7089LkvPv3BiBwks5OUwHTqvHso4aQvw%2Fl%2F955LX798%2FanO9Pf3n2RdDQm2C4%2B%2FMf789Zfh128EhgFkMRA%2B%2F%2FAro4ayGBtIL8uf33%2Fc%2F7Jxsnz58ocBHVg1HwLT8oJiKOK%2FQAQfF0ivL8ufX3803v5gYPr16y%2BGAfsqrMF09OQbGHIvvzAzAfUqAU35%2Ffv1%2Bx%2F%2F%2FzMyMjLgAD9%2FYbruF1AMqJcB5Iy7379%2BE2Nk48BjAKbr%2Fv%2F69Q%2Bo9wnICzv%2FvP9g%2Bl9QlA2XAdi8x%2Fjx4x%2Bg3j1MQFPmvn%2Fy7NfPn7%2F%2Fo4c2KEpBGF0c5Px3jx7%2FAuqdA05Iqo4Tm9mFhUvY5RQ5GIgAPx4%2F%2FPHrzetJt%2Ffnl8OTsqJtbx%2BrkEgmu6wCOyMTE%2Fbw%2BPeP4cfTRz9%2Bv3k1%2Ff7h4iKMzCRn2dXMxMpawCImycbCL8TCxM7OBAmwn%2F%2F%2BfHz%2F5%2FerF7%2F%2F%2Ffo56dHxsiqsuREEZMzaZYBUChAHALESVPghEG8GZecnpypR0jVAgAEAzPVNyh34CewAAAAASUVORK5CYII%3D';
var icon_misc = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8%2F9hAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAu1JREFUeNp0U3tIU1Ec%2FrbpplO3taUoynIGPlIpSAh6OQlR8hHDVWSWUSkFJkH5T%2F8GhpUWZZh%2F2JsyfCxS0kzLmYKarswoLMlnbs7nnM097na692oxND%2F4uPwe33fOuef8OIQQuKMx0E9Of87SVNGMWEkP0NTQLEs2mEfd%2BznuBm92bn%2FsMOqzNu9XYmPcVgiDAti8RW%2FEdE8fRhu1mLe7ag4O69VrDLTpyXXRhddTR1vfQWbshB%2BZg%2Bu3ia1xfcSANARfxr2gOKDGp0sF2rT2LiVbZAw0iqByfXc3sc7PEwY9N4pJd2I46d4dyvJzaiR5m5NFKJuNOB0OYtDpSHVs5EtGy9UEiOSeHrzcb%2FfKwBMIQFmtkEZFY2jE0T4%2BMHOB4Y%2B%2Bubrw47mwLy6Cw%2BXi1%2Fs2eFjMabRWgSp%2F0RXdKRX5eTKOtJw4TIZbWkjNjrgnjLs7m1KSSie6ukhn4WXyNXMb%2BZiTQWjtVQ8noBaJuSAGK3z1H9B2tLn5mH46C6uQWN%2BYVxsbGSgT2TL8Q33B9yWgtSquk0BOpsdA2SlYKAF4lKMe68Bq0DdQ8GJ7ycw4GK0HBQ5s8%2FTfJi44%2BWIw8Xpgai5PIRw2M2A3sTGzg1GL2c66CmUiUAJhxnoGXpvC0rk%2BwuXd0hpGy6XPUT1rcsGxRCfFciQ%2BrdzzPH5v1Wrxq0PqsoT7D9MlSWqYFxxgNLRWw7krk8j53oKRYKUS%2Byoesdc01d%2BPhswjHbapyVpGzPMWxqfUaNKlEREQiMXoKSlGX8k1%2BsptYexLLA8JLkuprDwj2xINL4kEutLbMPd2wGpaYFcXBQbA4vKE8tYdNh7TatGSf%2B7F6YHvqn9PuTIhoTa%2BqEg13PQalqpSbKBX4%2FsHsjX7lAGLgwOwhO9CVG4eWgsuNmX36pLWDFNFTMwD6%2BREdoAvIPEG%2BLzlvJ0%2BrGkJMC5yQHF4z%2FKNxsz%2FTiODm1KpYmWcmYlTrKSH%2Fo7z%2BdnZQff%2BPwIMADnNdR%2FaWF%2F8AAAAAElFTkSuQmCC';
var icon_help = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8%2F9hAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAopJREFUeNp0k19IU1Ecx3%2Fnur9KluafkKVLA4WQUbaI6MkejLAIqgchsH8vKSbMiBUY0kNImBlaFhgUBD1ED5Eklr301F%2FdFGEiOp06mzp1c%2FfPzr13p3NuOra5fvC5v8vv%2Fr7f87uHcxAhBAbHw7AVT17%2FyqHpMqWKcpDCUVyb9DVePLwCCYESDbpf%2FWgzGXUtleUF%2BsK8rIz83Vkcoj3Lq4K6HBRUtycgi5Lc1VR%2FpDXJYGAsBN0vv3VYLbsaqo%2FvNwU2VLQuqBAWVSC0KducATspRdk68vX7tDTpDT5runTUETeoaf7YWlaSe9tuLzO7fALIKoGfM9OJk4LdWgo6Oo%2BtOBPcLq84MbXy8MaVY62ouuF9kcmonzh10pblXpCoIcCwzwu%2F204kGVS1fYFDxfu0d5vFTD4PjfIRPlrOqbJytbJij8E1J6AoVgHLqiaufz4ZhwWrsW%2BMkVkeVZQVGJhWp8hKjWow6yIRJb5aXY8H0gUTa5k9sjOZ9rROwUpFUAIOYzWt6J3jgJbPdY4n1QORDI5qS6mLLC%2BvSYQghFLF%2FU6blmvb3dunwQpQLbAxpkReKEAGE4L%2FRDTNdATjGNXOs18YVNbW7SQn35DaZG3u13LJjsJtBigUUqh2iKMuL9bm%2FTgalcnWLm8x87hWI7XOxl%2F1zWGq7eM8nxr9Qpjvinino6mNibufSNjrlcQw%2F9TRcsajncTeAR88uP%2B2U5%2Bbd92412pEHJd%2BP2IxkBZ8kryy1HvrzgXtKHP%2F6gRuOs87pKVAx8boMC%2F4FzDmxZiixIAhC2JMXPTj8JiLl%2F4sPmK9TMPQJujpn4kv0n7vjYWma5SzlNLN8izlA7vOzrt1SZfkrwADADHTjb87FtdzAAAAAElFTkSuQmCC';
var icon_config = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8%2F9hAAAABmJLR0QA%2FwD%2FAP%2BgvaeTAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAB3RJTUUH1QkaDBM5i2PCSAAAAfBJREFUOMulkktoE2EUhb%2BZ%2BEyKTRQKgkqwzMaFtt1FrC40FGJm60JwIVSkqLUtElICFQNDQqBrQXRlQIriwomN0GJXgtI2iUkXFYJVadOXhiBERDozbmaGMR3rwrP7ueece%2B%2B5P%2FwnBOcjnVGigArI8Vgi9xdNNJ1RbI7YUlT7r%2FYDqKaZq%2Fj6tQHNbLQd6YxiNBp1I51RDPdaw6pFAcR0RolaZKur19vmZhwFePDwPvFYQgZyACKgDt4cMp4%2BmzAA9fatETbX15A6Jer1r%2Fdas4ndGRUsMYBgFW8MDBqatiXoum7oukZhfk4ovC8CyDsFK7R0sBHpu0i5UmG59gUgGY8l7v7zjE68yr80SpUS3Sd7KJYLmBNMArqrQTCSOgzUrPeVkE7XCYmjR47RbDZ5N%2FcWtzU8TvH4cJi%2BUCcdAS%2FZmU2Ot39LLn1eOtd9qoeAP8BKbfnyhfD5%2Bemp11XAABCDkVQXUHs0JjNbXmS2vEjHQR8A5t5yLv8CSZI4e7rX%2BmR2HiJQHB8OM%2FWmxJamI%2B7zs1Fv2iOaI8vZJ4850O7nTKgXYMxpAMDuXR72%2BA7x88cvsvkFgHCrSS6vUv1Y%2FSNsEWBl4zv7fQHa9np4PvMBIPxpcnTaSTRNkmvrqwtA0r5CMJK6BEw4uNvEO%2BE3N%2BLV9uq8VLwAAAAASUVORK5CYII%3D';

//Tango Icon Set
var icon_config = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8%2F9hAAAABmJLR0QA%2FwD%2FAP%2BgvaeTAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAB3RJTUUH1QkaDBM5i2PCSAAAAfBJREFUOMulkktoE2EUhb%2BZ%2BEyKTRQKgkqwzMaFtt1FrC40FGJm60JwIVSkqLUtElICFQNDQqBrQXRlQIriwomN0GJXgtI2iUkXFYJVadOXhiBERDozbmaGMR3rwrP7ueece%2B%2B5P%2FwnBOcjnVGigArI8Vgi9xdNNJ1RbI7YUlT7r%2FYDqKaZq%2Fj6tQHNbLQd6YxiNBp1I51RDPdaw6pFAcR0RolaZKur19vmZhwFePDwPvFYQgZyACKgDt4cMp4%2BmzAA9fatETbX15A6Jer1r%2Fdas4ndGRUsMYBgFW8MDBqatiXoum7oukZhfk4ovC8CyDsFK7R0sBHpu0i5UmG59gUgGY8l7v7zjE68yr80SpUS3Sd7KJYLmBNMArqrQTCSOgzUrPeVkE7XCYmjR47RbDZ5N%2FcWtzU8TvH4cJi%2BUCcdAS%2FZmU2Ot39LLn1eOtd9qoeAP8BKbfnyhfD5%2Bemp11XAABCDkVQXUHs0JjNbXmS2vEjHQR8A5t5yLv8CSZI4e7rX%2BmR2HiJQHB8OM%2FWmxJamI%2B7zs1Fv2iOaI8vZJ4850O7nTKgXYMxpAMDuXR72%2BA7x88cvsvkFgHCrSS6vUv1Y%2FSNsEWBl4zv7fQHa9np4PvMBIPxpcnTaSTRNkmvrqwtA0r5CMJK6BEw4uNvEO%2BE3N%2BLV9uq8VLwAAAAASUVORK5CYII%3D';
var icon_update = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8%2F9hAAAABHNCSVQICAgIfAhkiAAAAAlwSFlzAAAN1wAADdcBQiibeAAAABl0RVh0U29mdHdhcmUAd3d3Lmlua3NjYXBlLm9yZ5vuPBoAAAJhSURBVDiNhZNNaNRnEMZ%2F877%2Fj91V1lApiFjxoxQTK2ER0RaFXgWRYnoIwYVSDz148Oyp9OZRe%2BuhElgsLXiQXASvLSKILlHRVuqKVmtNiFnZ7Mf%2Fa8ZDjKzitnOe5zfMPPOImTGqmnX5EhirNWx2VI%2BMAjTrskG8a4GLrch31hr2%2FD8Bzbp8A5SBa8At8e7HsU8mZ4LyOlmav3pZC50C9gD7gVKtYecAgiHYBz6unHFRSfNuGxeW2Hzgi1C8Z%2Fn%2BncPWfdnzlarDVPJe5%2ByaaBhwzYVRNj59opT2%2B2iW4tIlKBJ2HpmOxHvCSpkHc790817n9%2FcBbmTddpi%2BXCLoPQQz%2FKHvAAh%2F%2Bx5MUZmgv%2FisAlxdE7nX%2B38oTs77qKS60saSAbr8CImqSFRFlx9hgxXyzjJBXMqcdz8167IZQG4e54SInB3buj3ctPvT2OUdrN0CzYln5gBIfj4K4pCxbWi8kcU%2F7mZLrb9yUz3tVr0QMMWSPjbo4HdNvREDxDNz%2BPGvsP4KlvQYtl7MbHUF4QcJ46nttT1hlC4STBwl2Ps1APmNWbLbF9H1W2ndvJ1q2r%2Biat%2FWGvbP8B%2BUgc7Htd0%2BSBbAlGByGjDy%2BV9Xp1Y%2F4s%2FrtwyzTbWGLbzrwl4fhlmgA59KFbWC0vxF0JzEVREvRMkKUSnupv3B58CldwEHTDW8f%2B9JZnmKiGPH%2BJZQQuHxvSepFjniQydoGTi4BnBDgBdaFKc0TT4ztXWmxYV%2Fn74YLDxrp1bkl02tolm6r8jyk8Dfbx3xfdWsywZx0sKIzez%2FwzQCcozVOJ8f1fMKwlMw2aBgNl0AAAAASUVORK5CYII%3D';
var icon_update_grey = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAQAAAC1%2BjfqAAAACXBIWXMAAA3XAAAN1wFCKJt4AAADGGlDQ1BQaG90b3Nob3AgSUNDIHByb2ZpbGUAAHjaY2BgnuDo4uTKJMDAUFBUUuQe5BgZERmlwH6egY2BmYGBgYGBITG5uMAxIMCHgYGBIS8%2FL5UBFTAyMHy7xsDIwMDAcFnX0cXJlYE0wJpcUFTCwMBwgIGBwSgltTiZgYHhCwMDQ3p5SUEJAwNjDAMDg0hSdkEJAwNjAQMDg0h2SJAzAwNjCwMDE09JakUJAwMDg3N%2BQWVRZnpGiYKhpaWlgmNKflKqQnBlcUlqbrGCZ15yflFBflFiSWoKAwMD1A4GBgYGXpf8EgX3xMw8BSMDVQYqg4jIKAUICxE%2BCDEESC4tKoMHJQODAIMCgwGDA0MAQyJDPcMChqMMbxjFGV0YSxlXMN5jEmMKYprAdIFZmDmSeSHzGxZLlg6WW6x6rK2s99gs2aaxfWMPZ9%2FNocTRxfGFM5HzApcj1xZuTe4FPFI8U3mFeCfxCfNN45fhXyygI7BD0FXwilCq0A%2FhXhEVkb2i4aJfxCaJG4lfkaiQlJM8JpUvLS19QqZMVl32llyfvIv8H4WtioVKekpvldeqFKiaqP5UO6jepRGqqaT5QeuA9iSdVF0rPUG9V%2FpHDBYY1hrFGNuayJsym740u2C%2B02KJ5QSrOutcmzjbQDtXe2sHY0cdJzVnJRcFV3k3BXdlD3VPXS8Tbxsfd99gvwT%2F%2FID6wIlBS4N3hVwMfRnOFCEXaRUVEV0RMzN2T9yDBLZE3aSw5IaUNak30zkyLDIzs%2BZmX8xlz7PPryjYVPiuWLskq3RV2ZsK%2FcqSql01jLVedVPrHzbqNdU0n22VaytsP9op3VXUfbpXta%2Bx%2F%2B5Em0mzJ%2F%2BdGj%2Ft8AyNmf2zvs9JmHt6vvmCpYtEFrcu%2BbYsc%2Fm9lSGrTq9xWbtvveWGbZtMNm%2FZarJt%2Bw6rnft3u%2B45uy9s%2F4ODOYd%2BHmk%2FJn58xUnrU%2BfOJJ%2F9dX7SRe1LR68kXv13fc5Nm1t379TfU75%2F4mHeY7En%2B59lvhB5efB1%2Flv5dxc%2BNH0y%2Ffzq64Lv4T8Ffp360%2FrP8f9%2FAA0ADzT6lvFdAAAAIGNIUk0AAHolAACAgwAA%2Bf8AAIDpAAB1MAAA6mAAADqYAAAXb5JfxUYAAAFaSURBVHjaXJHNK0RhFMZ%2F73vn3vlaKCVSYkKpiWKSj6zsbCR2VhoLzaX8G5ZYTFmws7fRxFqajWQWSoMSiVkozUdz7533fS0GY5yzOT09ndNzfhj%2BdmYps9auCEOr3A7xKMJ6MPve0oQB3DRR8hTkQd9qRBRzeoVRpohk9yAEYDqdnZCuEyJlWzwteDVHIrxd%2BDaIvBUsRmoEBPjMO5I4Z1Xv4tfAlWd%2F4qMZB%2FIYJJ8xLgEkuF3yKKRr%2BFRwcCg3p0Aeur1gldZFrjs54WiqKEaAWxpY9FuNRHnrtNI8gYdigDEEhiUKvGAwPzHdLrFvrSRtTYJx4JoHIhR8da43sq%2FNP0QpJy0FDGG4B6LcGNOTLYEEIGUFYFAUKaKBAKfKbCvmNPZdoJAM25Kir5BSRJnj5GfDh9puzJi4On6tv%2Fk6Z2JqsrHJ8y%2BLv7AImzZY%2FMO9nEm3K18DAOHGnx6T6PIeAAAAAElFTkSuQmCC';

//PC Icon set
var icon_donate = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8%2F9hAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAopJREFUeNqMkV9IU1Ecx3%2Fn3Lt7192wabqRYFmWvViuwkl%2F6CGI6I8Q6YMRQUOJ1lsPQiDke0JQPRiZCdGgh4IgoXwJegii7A%2BB1oOls9Sp21zb3Lz3nj%2Bde53ploQ%2F%2BHLO%2BZ3f93PO%2BR3EOYcXW8sPA0Cb0BkhQ%2BidUN%2BJSOyZGK39VjG0C%2B3GqrKB6cZwfv8%2BGqjaFHB6S1%2FVdwad7urNEjAOC9NxNtr%2FXI%2B9H7kjCh2Vxw9c3tJ0SHFVVkjYIcNCNM4%2BdfXmclNzzVJridblv36xwe0rleh8ClgqDQ4HRr4j9Y6FmeT%2BMv%2FOQO25o06UyWAyOQPmVBRkzJF7VxWefv1FkxmWWl3lGyUjGgOW1YFTCgg4YE2FmqZGBUsY5b6NAYkLuC5eJ26IXSo4rQM5BGTZ40YkkQKSzAAnVIjYIyTTgCSMuUmApLPAhZlTZrUEkJhj0TsBoHI2keaLc%2FOcLRpoCbAioASoAFgQey1MNgBjyCUzVAA%2ByISy%2FrnhsSslXo9sG%2BmSmZEVo53Ln26FpMgQ%2FTppCmQYPfSV7VE05U3dMb%2BGZYQ5YXnAMowUmBFGkPmdM398nui7EI2HUG6oDx6f6gipmnLj4LWrLnE7xMXdOGd2wzhbZUYAuk5ZYjZjbD%2FbctMbaOy0AVaET3Zcqmtrv603lCkvE4MI1hc9iOcbY8Xo0ycPehO3gudP18Bscva%2FTq%2FHC%2BGB7yCvTu5obnlr3u0OWuafsV92LhY3YOhj0v6BfXs94K1Q%2F9abVHxnEfieSRcLEuORLJgmA5NwiExkC%2Fas2mIAGKQQsK1aA1XFoChYzF3%2F1MrFAJPq9vuWo6ocwF%2B7dg9MOrImoKf70WBovb%2FwR4ABAFkqXFhfHjIDAAAAAElFTkSuQmCC';

//Silk Icon set
//var icon_donate = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8%2F9hAAAABGdBTUEAAK%2FINwWK6QAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAIISURBVDjLlZPRT1JxFMf5E%2B6fwL9Ri8lsI5cSV7swL82w0SZTB6zWXOuB0cLU8HKhuAooTNrFupcAsYWjh1sRIaDgTLGXxmubD2w%2B9Prth29tXMWH83J%2BZ5%2FvOd9zfhoAml7h%2Bmg3ReuhUxIdR37CrVanUXvgvvsOtk4kbJ%2BkEaos%2FbkSYCZv0wcri7%2FzrTS2f32AUOX%2B2nPWACvd1V4KmM7fnxQP1pE%2B2kSuJUM%2BEpFpvUOS5MJVHgQSuBCwWuU72eP3EA8TWCx523NFl%2BIv%2BzrxRgRr%2BwKeFJ1NVYA9y%2Bo3mjFskbkj9SDGpTGqm2dSJmosZfRYZXPClLxNqQJsGYt2bS%2BMbEtCF2SVmQCTukOPikaqbxPnik4l3ohC%2BilivbGKcC0Af%2FklXAVHczhuoC8FmDdpyl2YUrjyAlmfHytklATpJronwP9jAYbYIN3XHXTDuDGkJ6qeRzsz7XCNh1AjvshmRRXQnZWVmIQxOfTf5RFV%2Ffw3LyJkC%2B6d2U5PwOjbEe3Tz4%2FbQp0%2Fb92WY5VbsZtuQ3SQfpC71%2BR3%2FeAqr2ASR7I9AUSVepibUHhSFCVKQv31uXm%2B0nPwVQ5dgOfLM%2BjeXNdf6AFRnZz9NNVeKs8jtr%2BCCDHvRcmL8bSlqQtdo%2Fv%2BTBaZ%2BRrcXUaQqLMZy%2BGVf%2BOAcGPaWXCckW7OBgTdslrdPxtwvK6n%2FCCRAAAAAElFTkSuQmCC';

//var icon_viewed = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8%2F9hAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAnpJREFUeNrEU11IFFEU%2Fu7M7s4Mbg6u1oqmtAlZapGJaNIfSFQGFdESpNTLSlIPIShFoD0UVAiB9BRZRKGBCEYUpsWWJmQhpIaSrNqu5h%2BtbW7rNrPOzO06JRT05kPncjiXe8%2F3cc653yWUUqzEOKzQVkxg8dbKiJPiIQgirBZrHkfIdQIUA8RMWG6Rgios1BkGanWqIxZTEdMUWJYuCVscoc3RcNCtqSFoi5oJ0hlWY1tNB3gLRJtgrYmzJ9XYRPs%2Bdt1hVvCLwGiemRp1S%2FIaNElX0UnzsDFhDvF8BKX7D0G2OyDGpsCPt2LWewl8eLQ9Xk4tZTU2cTD0baHgpNuWdQLJVbNo%2F5KJosQRvK%2BvRNuVavi7WhCe6Uf3aBSuPWeRfSEIieXOBScaGTado7rSy%2BeUwX64EZZFQEQETjEMymoXJQHdXV0IjAygz%2BfHsH8ekhXIKHsAOfcUFha%2BBiybio5%2B%2Brylfj2W2uZ1KIwkqAhsoBwbkobOVy9AMrPhnaao98jmu1EDcB2owzSNvOS9bwNvTucMlKvOXeAlGT9UoNsXRUFxAfMipOQXoz%2FkxM6cTBzLT4XBCKI9lzH8qLKj8MzrClMLRwrsJb77u%2BnEYCudXVDp%2BdYATat8RjOq2sxY3TJGdXWeKr6HdLylhD69mNTOcC7%2F4zKQoYZCZHl6bAe3cntrjyc%2BSUpOx%2BrktRAFwVSZwXSgsZJD30KYDvjQNzRx23PXuPbx3o4x0eFiBHe2Q42EkXtu0MbyNzeUWyvSEoyThMC2rLYlKX1XyLvnH3Dzllfz9t7YMLXK4YTgWPdbbn9bCnPnP2QeZj7JPPrnIfnvv%2FGnAAMARWkCTIUPLEUAAAAASUVORK5CYII%3D';
//var icon_viewed_grey = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAFo9M%2F3AAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAntJREFUeNpi%2FP%2F%2FPwMYrF69%2Bq6wsPB%2FgABihIv4%2B%2Fv%2F%2F%2Fnz53%2BAAGIsKytjYGJi%2Bv%2Ft2zcGISEhBiZ5eXmGx48fzwEKMjQ0NEgDBBBCDxSwdHZ2rnr9%2BnXo79%2B%2FGURERBgZ7t69%2B19MTOy%2Fn5%2Ff%2F2fPnv1nBKpiKyws%2FAk09JSRkZE5QABhmIEOWEpLS41kZGTOHj16lOHAgQMM%2Bfn5DCCX%2BPj4MNTV1YEVMc2aNev%2F%2B%2Ffv%2F4eFhYHxjx8%2F%2Fi9duhRktCJBKwACiKACJgYCgKWxsZHh379%2F%2F79%2B%2FQqiGX79%2BsXAxcXFAAxa9y9fvuxievv27X%2BQy%2F%2F%2B%2Fcswb948UCgztLW1MfDw8OwEWi%2FHlJqaCtbFyMjIwMLCAooYBlAwJicnM8jKyi5gPnTo0GknJ6fo79%2B%2FgxWYmJiAjGfo6urapaam5soUHx%2B%2FzdTU1OTWrVu%2FJCQkGO7fv8%2BQkJAQCwx%2Fd6K8CRCgVSpWjRAKgiMeooiKVTB%2BRhqxjRCwlSCaOo34C5faI62FkPoipA4p0voLKogQtEhhY5EipJAju5J0B9dcMfDY93be7sycfHBSBo6MqqqQJIlxRcvsqH7NSzE54%2B%2F8Q%2FVHkurhXy7Ghi8JL9M03ZI0mOcZnD9d19elyVsYhsGP5XEct1VVbZmU7u%2Bo73mzLMtB0zSBxFgNSJIEpA6KouAwo2ka%2BL4P%2BgBRFMHzPIiiyJ7s%2B77fi8T6FgTBPSdeUZR1Am5yXReO48A0TXRdB1IVYRiupDQF2rZ9z7LsRsjzHGmaSuxJHMevlmXBtm3IsryKxONyCigRGIYBdV0%2FlWWZ0YQfTC4cEfaScHEkiF%2BET8L3WW38BQIuJq0FRBW8AAAAAElFTkSuQmCC';
//var icon_starred = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8%2F9hAAAABHNCSVQICAgIfAhkiAAAAAlwSFlzAAAChAAAAoQBpLDYYgAAABl0RVh0U29mdHdhcmUAd3d3Lmlua3NjYXBlLm9yZ5vuPBoAAAJwSURBVDiNjZJLSFRRGMd%2F59xz7zgPUSfR6GFuKihBa9MDihYRBLUpIso2UdEiiqzcRdC6x6YWbaSIjIwe2xZBGEXhwhZFJKVljkqjVnecGcfr3PO1cCZ6aHTg8Ifv%2FPh%2F38f%2FICLMd8eebm0d7zlw7l%2BMZp6TeqCalePdcHX0%2FETP3tb5uDkNUg%2BU60Tqb1U3HjKJJbtQmMv%2B6%2FbkfxugzNmq5ceaHOOi9Qzx2nV1Nvh6bU5URMpd48AmZSq3xRftOFHVuMdRNg9SRKwl8%2BWltZjrYgudybUdT34aDN1nn1u95nSkprnFq2py3Fg9jnHQxVGweQh9EEHcZQSBEExnKGYHv4b5z93hzLdLauhh9FX9%2BqstrpmYhW0ASiCcBlsAVd5UgVkA3kpEV1IMQga6j%2FTrXH6mzf9wO7TWBxkGNVHSL%2BBMgs6CzoHrgFcJpIEMmeFHjKSDK0pEeNsZObNwxZYLNQ21KBkrddSgypoApx5UDFFJJof7ePPibsfGI98Pa4BVrdMX%2FcHurmx6FPRnUP2gh0CNguODE4D%2BDk6eqfE%2B%2BnvvPO4diB79LYXBe3pxNLkkVdcSA8n%2BGtSsSAWYRlLP%2B%2By7gW%2FJrccnfQBTxhwtceUImEkgAFF%2FJB7MTiZTOpkI8uWq%2BYWo0ZEimPKbKpkowJZqRYwH1qoqYPwvA1OhwbUEviH3yZJJTeNVQGKpS7xB0F4I2mJFxeaaQBXGCkw9izM%2BUvjo56Qr7UduxiNhXV06OFjZF9mdbIglioUshSLy11duPxBbsHmNOjYwot6fvJx7AUQAr7RDuGG1Fz21X%2B9MxKje3lY4B%2BRFZOYH4WwtFoeWX0gAAAAASUVORK5CYII%3D';
//var icon_starred_grey = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAFo9M%2F3AAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAqxJREFUeNpi%2BP%2F%2FP8PBgwfrGECMhw8f%2FgcIIEYQAwRYQDwODg4GgAACS61cubLvyZMn%2F48cOeIIVvLu3bv%2FIBoowAAQQHA9MMC0bNky7hcvXvw%2FceLEX7CAjIzMl58%2FfzLIy8sz9fb2zmFYunSp%2Bdu3b%2F%2B%2FefMGbA5AAIFtgeE1a9ZcBtl26tQpIZgY4%2FLly8%2Fb29sb%2FPr1i%2BHfv38MMEt5eHgYQGJMERERhnv27JFhZWVlYGNjY2BnZwfjQ4cO%2FQPaz8gCUs3MzKx34cIFhmvXrnV8%2F%2F59h6Ki4hagIh6QHEAAobhh165d0ch8FEcCXc8KCh9g0L1EVsAEMgXoeQ9ZWdnvLCwsDEDjxYCBMAcWMozgMGZkBHNArgaxQQo5OTkZtm7depdp586dpSBJkBdhPgHhBw8eMCQkJKgwpaam9ly6dAksCfQN2AQQBmoUAEcciAAZB4o8WCCBaF5eXla4gi9fvoCC%2BT7QPd7AwCnX1taOB0YQJ0gOIMAw4hMGgD7Tl5CQOAM0meX3798xwOhYilUhesCBMFBzI9Dd%2F58%2Bffr%2F%2BfPn%2F69fv%2F7%2F8OHDq7CpZQQGcCQ%2FP3%2BxoKCgARAzc3Nzg0MBaCs4ZP7%2B%2FQtWCAoEULoCRiXD58%2Bf3wHpg8Bg7WUCMp4DNRjIyckxc3FxgV3148cPuEZQiDExMYFpYMAwiImJMaipqQkB1QZevHjRBB4GixcvXqGurh4O9DckBoEawFEN1IyMX758yQD0zh5g4vKYNWvWXyZYWADjqA6YW8DRAcKwpIkcfyBw48YNkNe8QJrh0QQFSnx8fGDNHz58AKcEYO4AxzEw6kB5CmwoSP7Pnz%2BSQPWPUKLRy8tL2NjYOBuo6faCBQuOA4XYgZgNpAaI%2FwIN4AwLC%2FMFBrJAQ0NDHVDsG1DvbwAQ1JoLo9cF5gAAAABJRU5ErkJggg%3D%3D';

// Fugue Icon set
//var icon_viewed_grey = '';

// WooFunction Icon Set
var icon_home = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAFo9M%2F3AAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAA8tJREFUeNpibPM0Y%2Fj%2F758jAzsLM%2BObmbb%2FAQKIESTy79dPBoa%2Ff%2F%2F%2BBwGAAGIAicxMCwNzZiYG%2FmdYt3Y1w6tXz%2F%2FfaPH%2B%2F%2F79m%2F8AAQTWw%2FD%2FPwMjExPQtL8MTP%2F%2B%2FxMo2nLif%2FKCzf%2FZJeQsmBRDUtfdmpHO8O3%2BBYZtV%2B%2BxMEkrqzL9%2BMnI8OfffwZhRVUWgABiBpnhrCrN4KQixRBqrvOqrmdil9Kr65Lzjl7cLP3qzj4GZxVph5nluX9Azvj6%2Fdf%2FL0D8%2BcuX%2F7%2F%2B%2Ff%2Ff7u%2Fwk8kqMuGAlJXz4R8%2FvjPcn1%2FA8HGSLcO76yeArvnDoJdW6s7IwMDAIi0tZcrOzi6VmZVdysTMwnDz%2BtUn%2B%2FcfWP3s2bNDAAEE8QcQgFT%2B%2BftPDEi%2FAnqJ4d%2Fv3wxMbOwMTAxQ8Pvv3wDL0KiXqvp6%2F5lFpQKjF277%2F%2F%2FPbwYWkOSfP3%2BW%2BlR3RxnaO4EVb%2BxrWff03HGIzhZ308ev37z%2B%2F%2B3bN6APfv%2F%2F%2BvPP%2F1%2B%2Ffv0%2FtnPr%2Fywr7akM3EygoPz%2F%2F9O3H%2F9fz3T8f6fb7v%2F33%2F%2F%2Bf%2Fz44T8LEyMDU35FBShuQNYw%2FPr0m%2BH%2FL2aG379%2FMXz5%2FJlh9959DEwG%2BvoMX758Agr%2BZPj4lYvh4%2Bf%2FDL9%2F%2FWBgZmZmaGxsBPuOU11dzUdQQEAhPCo6mBkYDn09Xd3s7Bz%2Fbt68uRkgQNNk7NJWFIXx7968p4k1tI2JInHQCkUqjREtLg5WcIiKdHGrrQouZilUSydbWjroEil1rqsgBvwHdCw6iENIVNrBKLEafTXP9%2FKSvHvjyROtZ7lwv3PuuZzvd%2B7mcD%2BElDjSrbZWX%2B2SsEUoJ%2BQ307QWmwJ1HngfdVOKFDltRxYsU7ktctH0MjmDuTj7%2BKSx%2Fkvv5JjSNTwKb40Hf1LJ2K%2Fl77Hj%2FeTZ26%2BxgLepGT%2FGX%2FUL82iDzQ%2F24Ld21RH0qMvB8Itw39Q7tLQ9%2B%2B%2FAvTAsC1vxFWyv%2FMSpbizCLn5g7Q2PR4I1at3I%2B7mxaDT60knMF3EYX4C8PARX3SgaOuoHpuF72kX8c2S1f5gYHpgWp8eqkvirrSeoaEjYbxzLiQGb2Jb6CQKlBBhRlrMfomTqdC9RoUNlZeymT5LpdGZTia%2BtOh75%2FX7k86bjkQ0XmJCwLwrkm4Vy1YMbnmjh8qKAC03Dwf4Bqt1uKHOfPnOatJydnWHPQx30WAnZbBalzlEY7tegzQErS2QKEkw7R7G6CqriQrgzzFOpPbBIJALOOatsMufUmTloQdJ36WQ3oJcrOaRz0tmt5tRdA7%2FS1cDBjYJbAAAAAElFTkSuQmCC';

//Dead Simple Icon Set
var icon_twitter = 'data:image/gif;base64,R0lGODlhDwAPANUAAJnO%2F9br%2F1Oj%2F8jh%2F9nw%2F0%2Bg%2F4fP%2F3PG%2F9js%2F8rl%2F8Hh%2F7La%2F%2Ff7%2F8vm%2F3nF%2F8bj%2F2W5%2F57P%2F8Xi%2F4XE%2F4TE%2F0qh%2F9zu%2F2y6%2F9rt%2F%2BDw%2F8nk%2F1%2Bv%2F8Hg%2F8Dg%2F0yk%2F%2FX6%2F8Pi%2F2i1%2F0ih%2F7fb%2F5%2FR%2F1ax%2F0ii%2F1q0%2F1Ot%2F0Kb%2F0We%2Fz2W%2F0yl%2F2fC%2F2C7%2F0%2Bp%2F2O%2B%2F124%2F2XA%2Fz%2BY%2F%2F%2F%2F%2FwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACH5BAAAAAAALAAAAAAPAA8AAAZ8QIKhRSwaWwbCQcZsOpsHmNShIEmvWJdWQvtov%2BCYuEFjiM%2Fok1pDa7stEPWpRH%2B47xd6CcUHtQGAABN8fC%2BGHG2GiossjQttIY2SkiaVFG4YCAgdHpUmKqAiGXdtG6AqKakpFREJAa8jqKkCM7W2t7YCAwUrvb6%2FKwUDQQA7';

//FamFamFam Icon Set
var icon_font = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8%2F9hAAAABGdBTUEAAK%2FINwWK6QAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAHJSURBVDjLY%2Fj%2F%2Fz8DJZiBZgY4tN9wcO6%2B0erZd2uKc%2BfNfoeWGxMcW27Msiq%2B3GWUdIZXL%2FokI14D7JqvB%2Bcsf3Rv4p6X%2F%2Ft3Pf%2Ffvf35%2F8Ilj3471V3bph9zmougC6xrr8mETbu7q3jl40%2FFKx5%2BLVzy8Ltd%2BeUZBvGnOYjygk3llfKCZY%2B%2Bu3fcWutcd21B07on%2F61yz88kKgwsCi8qJc%2B%2B9yhu2p37ppnnQ4C4oWblo%2F9WOReXEjTANOsCs1PD9VVZ8%2B9%2FN0k7m6Yfe5LLOPFMR%2BWyh%2F9dqq5eUvc6xIbXALOs8zEZc%2B9%2FC%2Bq%2BddEw%2FrSfXuRxLfP0swuqgAYEt934pOq2nxenAUbJZ0TjJt9%2BVbn80X%2Bv5huXrbLOb7LMOLfVterqjcYVj%2F%2BHtd38qey4TxqrAQaxpxntSy7PBvnVPO0MSmCZJ5%2FZWL7g%2Fv%2Bozlv%2Flex2K2EYoB9zigsYPS6lSx7%2Bj%2Bi59UYn6JgtTIGK635hdY%2FD9dnT7vxP6L%2F9X9F%2Bb4icxTYmFAMsMs6ti%2B2%2F9S9hwu3%2FAc3X32oHHOlVdtoroGS%2FR0vb9%2FAip8ILrwLrrv33rbn63zD02F5Zy22GtM8LdDMAACVPr6ZjGHxnAAAAAElFTkSuQmCC';

var menufont = "Arial";
if ($ljq.client.os == "Win") {
	menufont = "Verdana";
}

GM_addStyle(''
+'#GB_overlay {'
+'  background-image: url('+gb_overlay+');'
+'  position: fixed;'
+'  margin: auto;'
+'  top: 0;'
+'  left: 0;'
+'  z-index: 8000;'
+'  width:  100%;'
+'  height: 100%;'
//+'  display: none;';
+'	font-size:10px;'
+'	text-decoration:none;'
+'	text-underline-style:none;'
+'	color:#000000;'
+'	display:none;'
+'}'
+'#GB_window div, #GB_window span, #GB_window applet, #GB_window object, #GB_window iframe, #GB_window h1,'
+'#GB_window h2, #GB_window h3, #GB_window h4, #GB_window h5, #GB_window h6, #GB_window p, #GB_window blockquote,'
+'#GB_window pre, #GB_window a, #GB_window abbr, #GB_window acronym, #GB_window address, #GB_window big,'
+'#GB_window cite, #GB_window code, #GB_window del, #GB_window dfn, #GB_window em, #GB_window font, #GB_window img,'
+'#GB_window ins, #GB_window kbd, #GB_window q, #GB_window s, #GB_window samp, #GB_window small, #GB_window strike,'
+'#GB_window strong, #GB_window sub, #GB_window sup, #GB_window tt, #GB_window var, #GB_window dl, #GB_window dt,'
+'#GB_window dd, #GB_window ol, #GB_window ul, #GB_window li, #GB_window fieldset, #GB_window form, #GB_window label,'
+'#GB_window legend, #GB_window table, #GB_window caption, #GB_window tbody, #GB_window tfoot, #GB_window thead,'
+'#GB_window tr, #GB_window th, #GB_window td, #GB_window fieldset, #GB_window input, #GB_window p {'
+'	margin:0; padding:0; border:0; outline:0; font-size:12px; font-weight:normal; font-style:none;'
+'	font-family:helvetica, verdana, sans-serif; text-align:left; vertical-align:baseline; text-decoration:none;'
+'	float:none; style-list-type:none; background:#FFF; background-color:#FFF; color:#000;'
+'	position:relative; overflow:visible; width:auto; height:auto;'
+'	text-decoration:none; text-underline-style:none; outline:none;'
//font-size:100%;
+'}'
+'#GB_window :focus { outline:0; }'
//+'#GB_window body { line-height:1; color:black; background:white; }'
+'#GB_window ol, #GB_window ul { list-style:none; }'
+'#GB_window table { border-collapse:separate; border-spacing:0; }'
+'#GB_window caption, #GB_window th, #GB_window td { text-align:left; font-weight:normal; }'
+'#GB_window blockquote:before, #GB_window blockquote:after, #GB_window q:before, #GB_window q:after { content: ""; }'
+'#GB_window blockquote, #GB_window q { quotes: "" ""; }'
+'#GB_window {'
+'  top: 20px;'
+'  position: fixed;'
+'  background: #FFF;'
+'  border: 5px solid #AAA;'
+'	border-radius: 8px;'
+'  -moz-border-radius: 8px;'
+'	-webkit-border-radius: 8px;'
+'  overflow: auto;'
+'  width: 550px;'
+'  min-height: 150px;'
//+'	max-height: 300px;'
+'  z-index: 8050;'
+'	display:none;'
//+'	top: '+((winH/2)-200)+'px;'
//+'	left: '+((winW/2)-275)+'px;'
//+'	font-size:1em;'
+'}'
+'#GB_window input {'
//+'	border:1px solid #000;'
//+'	border:auto;'
+'	background-color:#EEE;'
+'	line-height:auto;'
+'	border: 2px #ffffff inset;'
+'}'
+'#GB_window #GB_caption {'
//+'  font: 14px bold helvetica, verdana, sans-serif;'
+'	font-size: 14px;'
+'  color: #FFF;'
+'	background:#888888;'
+'  padding: 2px 0 2px 5px;'
+'	margin:0;'
+'  text-align: left;'
+'}'
+'#GB_window #GB_caption span {'
+'	line-height:16px;'
+'	font-size:14px;'
//+'	font-size:14px'
+'	background:#888888;'
+'	font-weight:bold;'
+'	color:#FFF;'
+'}'
+'#GB_window img.close {'
+'  position: absolute;'
+'  top: 2px;'
+'  right: 5px;'
+'  cursor: pointer;'
+'  cursor: hand;'
+'}'
//#GB_body 
+'#GB_window #GB_body {'
//+'  font-size: 1.0em;'
+'  padding: 5px;'
//+'  display:none;'
//+'  font: 1em normal helvetica, verdana, sans-serif;'
+'	color:#000000;'
//+'	background:#000;'
+'	text-align:left;'
+'}'
//#GB_body 
+'#GB_window a:link, #GB_window a:visited, #GB_window a:active {'
//+'#GB_window #GB_body a:link, #GB_window #GB_body a:visited, #GB_window #GB_body a:active {'
+'  color:#000;'
//+'  text-decoration:underline;'
+'}'
//#GB_body 
+'#GB_window a:hover {'
+'  color:#556992;'
//+'  text-decoration:none;'
+'}'
+'#gmmnu {'
+'  background-color: #E9E6E5;'
+'	border-radius: 8px;'
+'	-moz-border-radius: 8px;'
+'	-webkit-border-radius: 8px;'
+'  position: fixed;'
+'  margin: auto;'
//+'  bottom: 5px;'
+'	bottom: '+quicklist+'px;'
+'  right: 5px;'
+'  z-index: 1000;'
//+'  width: 150px;'
+'  width: 180px;'
+'  min-height: 20px;'
+'  padding:5px;'
+'  text-align:left;'
+'  border:2px solid #333;'
//+'  font-family:Arial;'
+'  font-family:'+menufont+';'
+'  color:#333;'
+'	font-size:10px;'
+'	text-decoration:none;'
+'	text-underline-style:none;'
+'}'
+'#gmmnu a:link, #gmmnu a:visited, #gmmnu a:hover, #gmmnu a:active {'
+'	color:#264074;'
+'	text-decoration:none;'
+'	text-underline-style:none;'
+'	font-size:10px;'
+'	border-bottom:0;'
//+'	font-size:.9em;'
+'}'
//+'#gmmnuheader {text-align:center;font-weight:bold;font-size:10pt;margin-bottom:5px;}'
+'#gmmnu_title {text-align:center;font-weight:bold;font-size:10pt;line-height:16px;}'
+'#gmmnu_err {display:none;}'
+'#gmmnu_errmsg {font-size:9pt;margin-bottom:5px;}'
+'#gmmnu_links, '
+'#gmmnu_links_3d {'
//+'	display:none;'
+'	text-decoration:none;'
+'	min-height:20px;'
+'}'
+'#gmmnu_links_3d {'
+'	display:none;'
//+'	text-decoration:none;'
//+'	min-height:20px;'
+'}'
//+'#gmmnulinksheader, #gmmnuerrheader {'
+'#gmmnu .header {'
+'	position: relative;'
+'  font-weight:bold;'
+'  font-size:8pt;'
+'  margin:0 0 2px 0;'
//+'  padding:5px 0 0 0;'
+'  border-bottom:1px solid #000000;'
//+'	margin-bottom:5px;'
//+'	margin-top:5px;'
+'	display:block;'
+'  line-height: 20px;'
+'}'
+'#gmmnu .header .format_mp4 {'
+'	position: absolute;'
+'	top: 0;'
+'	right: 32px;'
+'  line-height: 18px;'
+'  font-size:11px;'
+'	display: inline;'
+'	margin-right: 2px;'
+'  font-size:10px;'
+'  font-weight:normal;'
+'	border-bottom:2px solid #000000;'
+'}'
+'#gmmnu .header .format_flv {'
+'	position: absolute;'
+'	top: 0;'
+'	right: 10px;'
+'  line-height: 18px;'
+'  font-size:10px;'
+'	display: inline;'
+'	margin-right: 2px;'
+'  font-size:11px;'
+'  font-weight:normal;'
+'	border-bottom:2px solid #d71e25;'
+'}'
+'#gmmnu .header .format_webm {'
+'	position: absolute;'
+'	top: 0;'
+'	right: 56px;'
+'  line-height: 18px;'
+'  font-size:11px;'
+'	display: inline;'
+'	margin-right: 2px;'
+'  font-size:10px;'
+'  font-weight:normal;'
+'	border-bottom:2px solid #acd147;'
+'}'
+'#gmmnu_linksfiles, '
+'#gmmnu_linksfiles_3d {'
+'  line-height: 20px;'
+'  font-size:11px;'
+'	vertical-align:middle;'
+'}'
/*
+'#gmmnu_linksfiles .link {'
+'	border-right:2px solid #336699;'
+'}'
+'#gmmnu_linksfiles .link {'
+'	border-right:2px solid #336699;'
+'}'
+'#gmmnu_linksfiles .link {'
+'	border-right:2px solid #336699;'
+'}'
+'#gmmnu_linksfiles .link {'
+'	border-right:2px solid #336699;'
+'}'
*/
+'#gmmnu_linksfiles .link a, #gmmnu_linksfiles .link a:visited, #gmmnu_linksfiles .link a:active, '
+'#gmmnu_linksfiles_3d .link a, #gmmnu_linksfiles_3d .link a:visited, #gmmnu_linksfiles_3d .link a:active {'
+'	position: relative;'
+'  line-height: 20px;'
+'	background-repeat:no-repeat;'
+'	background-position: left center;'
+'	font-weight:900;'
+'	padding-left:2px;'
+'	overflow:hidden;'
+'	display:block;'
+'  font-size:11px;'
//+'	text-decoration:none;'
//+'	text-underline-style:none;'
+'	white-space:nowrap;'
+'	vertical-align:middle;'
//+'	margin-bottom: 2px;'
+'	outline:none;'
//+'	border-right:2px solid #336699;'
+'}'
+'#gmmnu_linksfiles .link a.format, #gmmnu_linksfiles .link a.format:visited, #gmmnu_linksfiles .link a.format:active, '
+'#gmmnu_linksfiles_3d .link a.format, #gmmnu_linksfiles_3d .link a.format:visited, #gmmnu_linksfiles_3d .link a.format:active {'
+'	color:#000;'
+'}'
+'#gmmnu_linksfiles .link a[data-format="mp4"], '
+'#gmmnu_linksfiles_3d .link a[data-format="mp4"] {'
//+'	border-right:2px solid #000000;'
+'}'
+'#gmmnu_linksfiles .link a[data-format="flv"], #gmmnu_linksfiles .link a[data-format="svq"], '
+'#gmmnu_linksfiles_3d .link a[data-format="flv"], #gmmnu_linksfiles_3d .link a[data-format="svq"] {'
//+'	border-right:2px solid #d71e25;'
+'}'
+'#gmmnu_linksfiles .link a[data-format="webm"], '
+'#gmmnu_linksfiles_3d .link a[data-format="webm"] {'
//+'	border-right:2px solid #acd147;'
+'}'
+'#gmmnu_linksfiles .link a:hover, '
+'#gmmnu_linksfiles_3d .link a:hover {'
+'	background-color:#ccc;'
+'}'
+'#gmmnu_linksfiles .link a span.format, #gmmnu_linksfiles .link a:visited span.format, #gmmnu_linksfiles .link a:active span.format, '
+'#gmmnu_linksfiles_3d .link a span.format, #gmmnu_linksfiles_3d .link a:visited span.format, #gmmnu_linksfiles_3d .link a:active span.format {'
+'	position: absolute;'
+'	top: 0;'
+'	right: 0;'
+'  line-height: 20px;'
+'  font-size:11px;'
+'	display: inline;'
+'	margin-right: 2px;'
+'  font-size:11px;'
+'  font-weight:normal;'
+'}'
+'#gmmnu_linksfiles .link a span.size, '
+'#gmmnu_linksfiles_3d .link a span.size {'
+'	position: absolute;'
+'	top: 0;'
+'	right: 0;'
+'  line-height: 20px;'
+'  font-size:11px;'
+'	display: inline;'
+'	margin-right: 2px;'
+'  font-size:11px;'
+'  font-weight:normal;'
//+'  z-index: 1010;'
//+'  background-color: #E9E6E5;'
+'}'
+'#gmmnu_linksfiles .link .side_size, '
+'#gmmnu_linksfiles_3d .link .side_size {'
+'	position: absolute;'
+'	float:left;'
+'	left:-40px;'
+'	background-color: #E9E6E5;'
+'	padding-right:3px;'
+'	border:1px solid #000;'
+''
+''
+''
+'}'
+'#GB_settings {'
+'	'
+'}'
+'#GB_window ul.tabs {'
+'	margin: 0;'
+'	padding: 0;'
+'	float: left;'
//+'	width:500px;'
+'	list-style: none;'
//+'	list-style-type: none;'
+'	height: 32px;'
+'	border-bottom: 1px solid #999;'
+'	border-left: 1px solid #999;'
+'	width: 100%;'
+'	z-index:9000;'
+'	display:inline;'
+'	width: 100%;'
+'	clear:none;'
//+'	background: #e0e0e0;'
+'}'
+'#GB_window ul.tabs li {'
+'	float: left;'
+'	margin: 0;'
+'	padding: 0;'
+'	height: 31px;'
+'	list-style: none;'
//+'	list-style-type: none;'
+'	line-height: 31px;'
+'	border: 1px solid #999;'
+'	border-left: none;'
+'	margin-bottom: -1px;'
+'	background: #e0e0e0;'
+'	overflow: hidden;'
+'	position: relative;'
+'	z-index:9001;'
//+'	display:block;'
+'	clear:none;'
+'	display:block;'
+'}'
+'#GB_window ul.tabs li a {'
+'	text-decoration: none;'
+'	color: #000;'
+'	display: block;'
//+'	display:inline;'
+'	font-size: 1.2em;'
+'	padding: 0 15px 0 30px;'
+'	border: 1px solid #fff;'
+'	outline: none;'
//+'	background: #e0e0e0;'
+'	background: #e0e0e0;'
+'	z-index:9002;'
+'	clear:none;'
//+'	background-image:url('+icon_lists+');'
//+'	background-repeat:no-repeat;'
//+'	background-position: left center;'
+'}'
+'#GB_window ul.tabs li a.about {'
+'	background-image:url('+icon_about+');'
+'	background-repeat:no-repeat;'
+'	background-position: 8px 40%;'
+'	z-index:9003;'
+'	clear:none;'
//+'	margin-top:-2px;'
+'	padding-bottom:2px;'
+'	display:block;'
+'	line-height:31px;'
+'}'
+'#GB_window ul.tabs li a.vidopt {'
+'	background-image:url('+icon_font+');'
+'	background-repeat:no-repeat;'
+'	background-position: 8px 40%;'
+'	z-index:9003;'
+'	clear:none;'
//+'	margin-top:-2px;'
+'	padding-bottom:2px;'
+'	display:block;'
+'	line-height:31px;'
+'}'
+'#GB_window ul.tabs li a.misc {'
+'	background-image:url('+icon_misc+');'
+'	background-repeat:no-repeat;'
+'	background-position: 8px 40%;'
+'	z-index:9003;'
+'	clear:none;'
//+'	margin-top:-2px;'
+'	padding-bottom:2px;'
+'	display:block;'
+'	line-height:31px;'
+'}'
+'#GB_window ul.tabs li a.help {'
+'	background-image:url('+icon_help+');'
+'	background-repeat:no-repeat;'
+'	background-position: 8px 40%;'
+'	z-index:9003;'
+'	clear:none;'
//+'	margin-top:-2px;'
+'	padding-bottom:2px;'
+'	display:block;'
+'	line-height:31px;'
+'}'
//+'#GB_window li {'
//+'	background: #e0e0e0;'
//+'}'
+'#GB_window ul.tabs li a:hover {'
+'	background-color: #ccc;'
+'}'
//+'#GB_window #GB_body a:link, #GB_window #GB_body a:visited, #GB_window #GB_body a:active {'
//+'#GB_window ul.tabs li a:link {'
//+'  color:#0000EE;'
//+'  text-decoration:underline;'
//+'	background: #e0e0e0;'
//+'}'
//+'#GB_window html ul.tabs li.active, #GB_window html ul.tabs li.active a:hover  {'
+'#GB_window ul.tabs li.active, #GB_window ul.tabs li.active a:link, #GB_window ul.tabs li.active a:hover  {'
+'	background: #fff;'
+'	border-bottom: 1px solid #fff;'
+'}'
+'#GB_window ul.tabs li.active, #GB_window ul.tabs li.active a.about:link, #GB_window ul.tabs li.active a.about:hover {'
+'	background-image:url('+icon_about+');'
+'	background-repeat:no-repeat;'
+'	background-position: 8px 40%;'
+'}'
+'#GB_window ul.tabs li.vidopt, #GB_window ul.tabs li.active a.vidopt:link, #GB_window ul.tabs li.active a.vidopt:hover {'
+'	background-image:url('+icon_font+');'
+'	background-repeat:no-repeat;'
+'	background-position: 8px 40%;'
+'}'
+'#GB_window ul.tabs li.active, #GB_window ul.tabs li.active a.misc:link, #GB_window ul.tabs li.active a.misc:hover {'
+'	background-image:url('+icon_misc+');'
+'	background-repeat:no-repeat;'
+'	background-position: 8px 40%;'
+'}'
+'#GB_window ul.tabs li.active, #GB_window ul.tabs li.active a.help:link, #GB_window ul.tabs li.active a.help:hover {'
+'	background-image:url('+icon_help+');'
+'	background-repeat:no-repeat;'
+'	background-position: 8px 40%;'
+'}'
+'#GB_window .tab_container {'
+'	border: 1px solid #999;'
+'	border-top: none;'
//+'	border-bottom: none;'
+'	clear: both;'
+'	float: left; '
//+'	position:relative;'
+'	width: 100%;'
+'	background: #fff;'
+'	margin-bottom:5px;'
+'}'
+'#GB_window .tab_content {'
+'	padding: 20px;'
//+'	font-size: 1.2em;'
+'	position:relative;'
+'	display:block;'
+'}'
+'#GB_window .tab_content a:link, #GB_window .tab_content a:visited, #GB_window .tab_content a:hover, #GB_window .tab_content a:active {'
+'	color:#FA0000;'
+'	border-bottom:1px dashed #FF0000;'
//+'	text-decoration:#EEE solid;'
//+'	text-underline-style:dashed;'
+'}'
+'#GB_window .tab_container img {'
+'	vertical-align:middle;'
+'	text-align:center;'
+'}'
+'#GB_window .tab_content h2 {'
+'	font-weight: normal;'
+'	padding-bottom: 5px;'
+'	margin-bottom: 10px;'
+'	border-bottom: 1px dashed #ddd;'
+'	font-size: 14px;'
+'}'
+'#GB_window .tab_content h3 a {'
+'	color: #254588;'
+'}'
//+'#GB_window p {'
//+'	font-size: 12px;'
//+'}'
//+'#GB_body .tab_content img {'
//+'	float: left;'
//+'	position:relative;'
//+'	margin: 0 20px 20px 0;'
//+'	border: 1px solid #ddd;'
//+'	padding: 5px;'
//+'}'
//+'#GB_trackers {'
//+'	'
//+'}'
//+'#GB_lists {'
//+'	'
//+'}'
//+'#GB_list-tracker {'
//+'	'
//+'}'
+'#GB_initialize {'
+' display:none;'
+'}'
+'#gmmnu_menu {'
+'	margin-bottom:-2px;'
+'  line-height: 24px;'
+'	height: 24px;'
+'	font-family:Arial;'
+'  text-align:left;'
//+'  font-size:10px;'
+'	display:block;'
+'	outline:none;'
+'	padding-bottom:1px;'
+'}'
+'#gmmnu_menu .menubar .config {'
+'	float: left;'
+'	margin-bottom:-2px;'
//+'	margin: 0 1px -2px 1px;'
+'	padding: 0 1px 0 1px;'
+'  line-height: 24px;'
+'	height: 24px;'
+'	background-image:url('+icon_config+');'
+'	background-repeat:no-repeat;'
+'	background-position: center center;'
+'	display:inline;'
+'	outline:none;'
+'	width: 20px;'
+'}'
+'#gmmnu_menu .menubar .update {'
+'	float: left;'
+'	margin-bottom:-2px;'
//+'	margin: 0 1px -2px 1px;'
+'	padding: 0 1px 0 1px;'
+'  line-height: 24px;'
+'	height: 24px;'
+'	background-image:url('+icon_update+');'
+'	background-repeat:no-repeat;'
+'	background-position: center center;'
+'	display:inline;'
+'	outline:none;'
+'	width: 20px;'
//+'	visibility: hidden;'
//+'	display: none;'
+'}'
+'#gmmnu_menu .menubar .update_grey {'
+'	float: left;'
+'	margin-bottom:-2px;'
//+'	margin: 0 1px -2px 1px;'
+'	padding: 0 1px 0 1px;'
+'  line-height: 24px;'
+'	height: 24px;'
+'	background-image:url('+icon_update_grey+');'
+'	background-repeat:no-repeat;'
+'	background-position: center center;'
+'	display:inline;'
+'	outline:none;'
+'	width: 20px;'
+'}'
+'#gmmnu_menu .menubar .donate {'
+'	float: left;'
+'	margin-bottom:-2px;'
//+'	margin: 0 1px -2px 1px;'
+'	padding: 0 1px 0 1px;'
+'  line-height: 24px;'
+'	height: 24px;'
+'	background-image:url('+icon_donate+');'
+'	background-repeat:no-repeat;'
+'	background-position: center center;'
+'	display:inline;'
+'	outline:none;'
+'	width: 20px;'
+'}'
/*
+'#gmmnu_menu .menubar .viewed {'
+'	float: left;'
+'	margin-bottom:-2px;'
//+'	margin: 0 1px -2px 1px;'
+'	padding: 0 1px 0 1px;'
+'  line-height: 24px;'
+'	height: 24px;'
+'	background-image:url('+icon_viewed+');'
+'	background-repeat:no-repeat;'
+'	background-position: center center;'
+'	display:inline;'
+'	outline:none;'
+'	width: 20px;'
+'}'
+'#gmmnu_menu .menubar .viewed_grey {'
+'	float: left;'
+'	margin-bottom:-2px;'
//+'	margin: 0 1px -2px 1px;'
+'	padding: 0 1px 0 1px;'
+'  line-height: 24px;'
+'	height: 24px;'
+'	background-image:url('+icon_viewed_grey+');'
+'	background-repeat:no-repeat;'
+'	background-position: center center;'
+'	display:inline;'
+'	outline:none;'
+'	width: 20px;'
+'}'
+'#gmmnu_menu .menubar .starred {'
+'	float: left;'
+'	margin-bottom:-2px;'
//+'	margin: 0 1px -2px 1px;'
+'	padding: 0 1px 0 1px;'
+'  line-height: 24px;'
+'	height: 24px;'
+'	background-image:url('+icon_starred+');'
+'	background-repeat:no-repeat;'
+'	background-position: center center;'
+'	display:inline;'
+'	outline:none;'
+'	width: 20px;'
+'}'
+'#gmmnu_menu .menubar .starred_grey {'
+'	float: left;'
+'	margin-bottom:-2px;'
//+'	margin: 0 1px -2px 1px;'
+'	padding: 0 1px 0 1px;'
+'  line-height: 24px;'
+'	height: 24px;'
+'	background-image:url('+icon_starred_grey+');'
+'	background-repeat:no-repeat;'
+'	background-position: center center;'
+'	display:inline;'
+'	outline:none;'
+'	width: 20px;'
+'}'
*/
+'#gmmnu_menu .menubar .home {'
+'	float: left;'
+'	margin-bottom:-2px;'
//+'	margin: 0 1px -2px 1px;'
+'	padding: 0 1px 0 1px;'
+'  line-height: 24px;'
+'	height: 24px;'
+'	background-image:url('+icon_home+');'
+'	background-repeat:no-repeat;'
+'	background-position: center center;'
+'	display:inline;'
+'	outline:none;'
+'	width: 20px;'
+'}'
//twitter icon 15x15 tweak height - donation hover makes it move 1px down
+'#gmmnu_menu .menubar .twitter {'
+'	float: left;'
+'	margin-bottom:-2px;'
//+'	margin: 0 1px -2px 1px;'
+'	padding: 0 1px 0 1px;'
+'  line-height: 23px;'
+'	height: 23px;'
+'	background-image:url('+icon_twitter+');'
+'	background-repeat:no-repeat;'
+'	background-position: center center;'
+'	display:inline;'
+'	outline:none;'
+'	width: 20px;'
+'}'
+'#gmmnu_menu .menubar {'
+'	display: block; height: 24px; line-height: 24px; margin-left: -3px; margin-right: -3px;'
+'}'
+'#gmmnu_menu .menubar div :hover {'
//+'	background-color: #CCC;'
+'}'
+'#gmmnu #gmmnu_donations {'
+'	position:relative;'
//+'	display: none; margin: 0 -5px -5px -5px; padding:5px 5px 5px 5px;'
+'	display:none;'
+'}'
+'#gmmnu #gmmnu_donations .icon_donate_paypal, #GB_window .icon_donate_paypal {'
+'	margin-top:5px;'
//+'	margin-bottom:-2px;'
//+'	margin: 0 1px -2px 1px;'
+'	padding: 0 1px 0 1px;'
+'  line-height: 48px;'
+'	height: 48px;'
+'	background-image:url('+icon_donate_paypal+');'
+'	background-repeat:no-repeat;'
+'	background-position: center center;'
+'	text-align:center;'
+'	display:block;'
+'	outline:none;'
+'	width: 75px;'
+'	text-decoration:none;'
+'}'
+'#gmmnu #gmmnu_donations .icon_donate_flattr, #GB_window .icon_donate_flattr {'
+'	margin-top:5px;'
//+'	margin-bottom:-2px;'
//+'	margin: 0 1px -2px 1px;'
+'	padding: 0 1px 0 1px;'
+'  line-height: 20px;'
+'	height: 20px;'
+'	background-image:url('+icon_donate_flattr+');'
+'	background-repeat:no-repeat;'
+'	background-position: center center;'
+'	text-align:center;'
+'	display:block;'
+'	outline:none;'
+'	width: 75px;'
+'	text-decoration:none;'
+'}'
+'	#gm_tab_firefox img, #gm_tab_chrome img {'
+'	background-color:#CCCCCC;'
+'	padding:1px;'
+'	border:1px solid #909090'
+'}'
+'');

var GB_DONE = false;
var GB_HEIGHT = 100;
var GB_WIDTH = 550;

function GB_show(caption, body, height, width) {
	//var winW = window.innerWidth;
	//winH = window.innerHeight;
	//var newW = (winW/2)-275;
	//style='left:"+newW+"px'
  
	GB_HEIGHT = height || 100;
	GB_WIDTH = width || 550;
	if(!GB_DONE) {
		$ljq("#GB_initialize").html("<div id='GB_overlay'></div><div id='GB_window'>\n<div id='GB_caption'>"+caption+"</div>\n"
			+ "<img class='close' src='"+gb_close+"' alt='Close window'/>\n<div id='GB_body'>\n"+body+"\n</div></div>");
		$ljq("#GB_window img").click(GB_hide);
		$ljq("#GB_overlay").click(GB_hide);
		$ljq(window).on("resize", function() {
			GB_position();
		});
	GB_DONE = true;
	}
  
	$ljq(document).on('keydown.facebox', function(e) {
	if (e.keyCode == 27) {
		GB_hide();
	}
	return true;
	});

	$ljq("#GB_initialize").show();
  
	$ljq("#GB_overlay").show();
	GB_position();

	$ljq("#GB_window").show();
}

function GB_hide() {
	$ljq("#GB_initialize").html('');
	$ljq("#GB_window,#GB_overlay,#GB_initialize").hide();
	//GM_log('unbind!');
	$ljq(window).off();
	if (videolistRefresh) {
		videolistRefresh = false;
		generate_links();
	}
}

function GB_position() {
	//GM_log('resized!');
	var de = document.documentElement;
	var w = self.innerWidth || (de&&de.clientWidth) || document.body.clientWidth;
	var h = self.innerHeight || (de&&de.clientHeight) || document.body.clientHeight;
	//$ljq("#GB_window").css({width:GB_WIDTH+"px",height:GB_HEIGHT+"px",
	//left: ((w - GB_WIDTH)/2)+"px" });
	$ljq("#GB_window").css({left: ((w - GB_WIDTH)/2)+"px" });
	//$ljq("#GB_frame").css("height",GB_HEIGHT - 32 +"px");
}

//Vars used throughout script
var pattern = '';
var matches = '';

//Debugging
//GM_setValue('lastCheck', 0);
//GM_setValue('lastVersion', 0);
var lastCheck = GM_getValue('lastCheck', 0);
var lastVersion = GM_getValue('lastVersion', 0);
//alert(lastCheck+" "+lastVersion);
GMlog("lastCheck: "+lastCheck+" lastVersion: "+lastVersion);
var d = new Date();
var currentTime = Math.round(d.getTime() / 1000); // Unix time in seconds
var name = "";
var ver = "";
var notes = "";
var	running_version = '';
var	latest_version = '';
var videoLength = "";
var newtitle = "";

function updateCheck() {
	GM_xmlhttpRequest({
    method: 'GET',
    url: 'https://userscripts.org/scripts/source/'+scriptId+'.meta.js',
    //headers: {'User-agent': 'Mozilla/4.0 (compatible) Greasemonkey','Accept': 'text/plain'},
    onload: function(res) {
    if (res.status == '200') {
	try {
    var text = res.responseText;
    //alert(text);
    GMlog(text);
    
	pattern = /(@name(\t+|\s+)?(.*)(\r|\n)?)/ig;
	matches = text.match(pattern);
	//alert(matches[0].replace(pattern, "$3"));
	name = matches[0].replace(pattern, "$3");
	
    pattern = /(@version(\t+|\s+)?(.*)(\r|\n)?)/ig;
	matches = text.match(pattern);
	//alert(matches);
	//alert(matches[0].replace(pattern, "$3"));
	ver = matches[0].replace(pattern, "$3");
	ver = $ljq.trim(ver);
	
	pattern = /(@notes(\t+|\s+)?(.*)(\r|\n)?)/ig;
	matches = text.match(pattern);
	//alert(matches[0].replace(pattern, "$3"));
	notes = matches[0].replace(pattern, "$3");
	
	running_version = parseVersionString(scriptVersion);
	latest_version = parseVersionString(ver);
	//alert(running_version.major+'.'+running_version.minor+'.'+running_version.patch+' '+latest_version.major+'.'+latest_version.minor+'.'+latest_version.patch);
	
	var showupdatealert = false;
	//alert(running_version.major+'.'+running_version.minor+'.'+running_version.patch+' '+latest_version.major+'.'+latest_version.minor+'.'+latest_version.patch);
	if (running_version.major < latest_version.major) {
	    // A major new update is available!
	    showupdatealert = true;
	    //alert("Major update");
	} else if (running_version.minor < latest_version.minor) {
	    // A new minor update is available.
	    if (running_version.major <= latest_version.major) {
	    	showupdatealert = true;
	    	//alert("Minor update");
	    }
	} else if (running_version.patch < latest_version.patch){
	    // A new patch update is available.
	    if (running_version.major <= latest_version.major) {
	    	if (running_version.minor <= latest_version.minor) {
	    		showupdatealert = true;
	    		//alert("Patch update");
	    	}
	    }
	}

    if (showupdatealert) {
		showupdatealert = false;
		lastVersion = parseVersionString(lastVersion.toString());
		//alert(lastVersion.major+'.'+lastVersion.minor+'.'+lastVersion.patch+' '+latest_version.major+'.'+latest_version.minor+'.'+latest_version.patch);
		if (lastVersion.major < latest_version.major) {
		    // A major new update is available!
		    showupdatealert = true;
		    //alert("Major update");
		} else if (lastVersion.minor < latest_version.minor) {
		    // A new minor update is available.
		    if (lastVersion.major <= latest_version.major) {
		    	showupdatealert = true;
		    	//alert("Minor update");
		    }
		} else if (lastVersion.patch < latest_version.patch){
		    // A new patch update is available.
		    if (lastVersion.major <= latest_version.major) {
		    	if (lastVersion.minor <= latest_version.minor) {
		    		showupdatealert = true;
		    		//alert("Patch update");
		    	}
		    }
		}
		
		if (showupdatealert) {
			//Show update alert and info
			//alert("alertUpdate");
			//alertUpdate(scriptId, name, ver, notes);
			$ljq("#gmmnu_menu .menubar #update_icon a").removeClass('update_grey').addClass('update').attr("data-title", "Update to v"+ver);
		} else {
			//Dont check for 24 hours
			//alert("lastCheck");
			GM_setValue('lastCheck', currentTime);
		}
	} else {
		//Dont check for 24 hours
		//alert("lastCheck");
		GM_setValue('lastCheck', currentTime);
	}
	
	if (debugUpdate == true) {
		alertUpdate(scriptId, name, ver, notes);
	}
	
	} catch(e) {} //try
	} //status
    } //onload
	});
}

function alertUpdate(scriptId, name, ver, notes) {
	//alert(scriptId+"\n"+name+"\n"+ver+"\n"+notes);
	GB_DONE = false;
	//running_version = parseVersionString(scriptVersion);
	//latest_version = parseVersionString(ver);
	
	var text = '<div class="tab_content" style="padding:0;">';
	text += '<div style="margin:0 auto; text-align:center; font-size:13px;">There is an update available for &quot;'+scriptName+'&quot;</div>';
	text += '<div style="clear:both; margin-bottom:10px;"></div>';

	text += '<div style="margin:0 auto; text-align:center;"><strong style="font-size: 14px; font-weight:bold;">Upgrade to the Latest</strong></div>';
	text += '<div style="clear:both; margin-bottom:5px;"></div>';
	
	// vertical-align:middle;
	text += '<div style="width:170px; height:75px; line-height:75px; margin:0 auto;">';
	//text += '<span style="height:69px; line-height:69px; margin-top:-20px;">-></span>';
	//text += '<div style="position:relative; float:left; width:50px; height:50px; line-height:50px; background-color:#eee; border: 2px solid #aaa; -moz-border-radius: 5px; vertical-align:middle;">';
		text += '<div style="position:relative; float:left; width:65px; height:75px; line-height:75px; background-image: url(\''+update_grey+'\'); background-repeat:no-repeat; background-position: top left; vertical-align:middle; text-align:center">';
			text += '<div style="margin-top:-5px; margin-left:17px; background:none; font-size:13px;">'+running_version.major+'.'+running_version.minor+'.'+running_version.patch+'</div>';
			//text +=	'<div>'+running_version.major+'.'+running_version.minor+'.'+running_version.patch+'</div>';
			//text +=	'<div style="float:left; background:none;">'+running_version.major+'.'+running_version.minor+'.'+running_version.patch+'</div>';
		text += '</div>';
		//text += '<div style="float:left;">x</div>';
		text += '<div style="position:relative; float:right; width:65px; height:75px; line-height:75px; background-image: url(\''+update_green+'\'); background-repeat:no-repeat; background-position: top left; vertical-align:middle;">';
			text += '<div style="margin-top:-5px; margin-left:17px; background:none; font-size:13px;">'+latest_version.major+'.'+latest_version.minor+'.'+latest_version.patch+'</div>';
		text += '</div>';
	text += '</div>';
	//text += '';
	text += '<div style="clear:both;"></div>';
	
	text += '<div style="width:150px; margin:0 auto; line-height:30px; vertical-align:middle;">';
		text += '<div style="position:relative; float:left; width:50px; vertical-align:middle;">';
			text += '<div style="margin-top:-10px; margin-left:-2px; font-size: 13px;">Installed</div>';
		text += '</div>';
		text += '<div style="position:relative; float:right; width:60px; vertical-align:middle;">';
			text += '<div style="margin-top:-10px; margin-left:6px; color:#198835; font-size:13px; text-align:center;">Available</div>';
		text += '</div>';
	text += '</div>';
	//text += '<hr/><br/>';
	text += '<div style="clear:both; height:1px; background-color:#000000; margin-top:5px; margin-bottom:5px;"></div>';
	
	//-moz-border-radius: 5px;
	//text += '<div style="min-width:20px; height:20px; line-height:20px; background-color:#00FF00; border:1px solid #00FF00; -moz-border-radius: 5px;">';
	text += '<div style="margin:0 auto; text-align:center;">';
	text += '<strong style="font-size: 14px; font-weight:bold;">What would you like to do?</strong>';
	text += '</div>';
	//text += '</div>';
	//text += '<br/>';
	text += '<div style="clear:both; margin-bottom:5px;"></div>';
	text += '<div style="width:330px; margin:0 auto; vertical-align:middle;">';
	text += '<div style="position:relative; float:left; width:160px; vertical-align:middle; text-align:right; display: block; line-height: 22px;">';
		text += '<a href="http://userscripts.org/scripts/show/'+scriptId+'" title="http://userscripts.org/scripts/show/'+scriptId+'" style="font-size: 13px; color:#0000EE; border-bottom:1px dashed #AAA;">Go to Script Homepage</a>';
	text += '</div>';
	//text += '<br/>';
	text += '<div style="position:relative; float:right; width:160px; vertical-align:middle; text-align:left; display: block; line-height: 22px;">';
		text += '<a href="http://userscripts.org/scripts/source/'+scriptId+'.user.js" style="color: #198835; border-bottom:1px dashed #AAA; font-size: 13px;">Upgrade to '+latest_version.major+'.'+latest_version.minor+'.'+latest_version.patch+' now</a>';
	text += '</div>';
	//text += '<div style="clear:both;></div>';
	text += '<br/>';
	text += '<div style="position:relative; float:left; width:160px; vertical-align:middle; text-align:right; display: block; line-height: 22px;">';
		text += '<a href="#" id="gb_update_wait" style="font-size: 13px; color:#0000EE; border-bottom:1px dashed #AAA;">Remind me tomorrow</a>';
	text += '</div>';
	//text += '<br/>';
	text += '<div style="position:relative; float:right; width:160px; vertical-align:middle; text-align:left; display: block; line-height: 22px;">';
		text += '<a href="#" id="gb_update_waitnextversion" style="font-size: 13px; color:#0000EE; border-bottom:1px dashed #AAA;">Skip this version</a>';
	text += '</div>';
	text += '</div>';
	//text += '<br/>';
	text += '<div style="clear:both; margin-bottom:2px;"></div>';
	text += '<div style="clear:both; height:1px; background-color:#000000; margin-top:5px; margin-bottom:5px;"></div>';
	
	text += '<div style="margin:0 auto; text-align:center;">';
	text += '<strong style="font-size: 14px; font-weight:bold;">Donations and Reviews</strong>';
	text += '</div>';
	text += '<div style="clear:both; margin-bottom:5px;"></div>';
	text += '<div style="text-align:center; font-size: 12px; line-height:16px;">';
	text += 'Please consider Donating or writing a Review if you enjoy using this userscript.<br/>'
	text += 'Donation options can be found in the menu or the Configuration overlay.';
	text += '</div>';
	text += '<div style="clear:both; margin-bottom:2px;"></div>';
	text += '<div style="clear:both; height:1px; background-color:#000000; margin-top:5px; margin-bottom:5px;"></div>';
	
	text += '<div style="margin:0 auto; text-align:center;">';
	text += '<strong style="font-size: 14px; font-weight:bold;">Update Notes</strong>';
	text += '</div>';
	text += '<div style="clear:both; margin-bottom:5px;"></div>';
	text += '<div style="text-align:left; font-size: 12px; line-height:16px;">';
	text += notes.replace(/\\/g, '');
	text += '</div>';
	text += '</div>';
	
	//$ljq("#GB_body").html(ver+'<br/>'+scriptId+'<br/>'+name+'<br/>'+notes+'<br/>'+ver+'<br/>'+scriptId+'<br/>'+name+'<br/>'+notes+'<br/><br/><br/>'+ver+'<br/>'+scriptId+'<br/>'+name+'<br/>'+notes+'<br/>');
	//$ljq("#GB_body").html(text);

	var t = '<span style="font-weight:bold;">Update Alert!</span> - '+scriptName;
	//name
    //GB_show(t,u,600,600);
    GB_show(t,text,0,550);
    //return false;    
    //alert(latest_version.major+'.'+latest_version.minor+'.'+latest_version.patch);
    
    //Initial display of content - check height
	tab_content_height();
}

/*
function vidopt_update_title(overlay) {	
	if (overlay) {
		//alert("true");
		//alert($ljq(this).val());
		//load_list($ljq(this).val());
		newtitle = $ljq("#gm_settings_vidopt_title").val();
		newtitle = newtitle.replace(/%T/g, title).replace(/%U/g, user).replace(/%Y/g, year).replace(/%M/g, month).replace(/%D/g, day).replace(/%V/g, vcode).replace(/%L/g, videoLength).replace(/%%/g, "%");
		$ljq('#gm_tab_vidopt #dltitle').html(newtitle);
	} else {
		//alert("false");
		newtitle = GM_getValue('gm_title', '');
		newtitle = newtitle.replace(/%T/g, title).replace(/%U/g, user).replace(/%Y/g, year).replace(/%M/g, month).replace(/%D/g, day).replace(/%V/g, vcode).replace(/%L/g, videoLength).replace(/%%/g, "%");
		generate_links();
	}
}
*/

function showConfiguration() {
	GB_DONE = false;
	
	var t = '<span>'+scriptName+' - Configuration...</span>';
	var text = ''
		+'<ul class="tabs">'
		+'<li><a href="#gm_tab_about" class="about">About</a></li>'
		+'<li><a href="#gm_tab_vidopt" class="vidopt">Video Title</a></li>'
		+'<li><a href="#gm_tab_help" class="help">Help</a></li>'
		+'</ul>'
		+'<div class="tab_container">'
			+'<div id="gm_tab_about" class="tab_content">'
			+'	<h2>'+scriptName+' [v'+scriptVersion+'] '
			+'	<a href="http://userscripts.org/scripts/show/'+scriptId+'">http://userscripts.org/scripts/show/'+scriptId+'</a></h2>'
			+'	<p>'
			+'	<div style="display:block; position:relative; height:30px; line-height:18px;">'
			+'		<div style="display:inline; position:relative; float:left; width:90px; text-align:center; vertical-align:bottom;">'
			+'			<center><a href="'+donationLink_flattr+'" target="_blank" class="icon_donate_flattr">'
			//+'			<img src="'+icon_donate_flattr+'">'
			+'			</a></center>'
			+'				<div style="height:10px; clear:both;"></div>'
			+'			<center><a href="'+donationLink_paypal+'" target="_blank" class="icon_donate_paypal">'
			//+'			<img src="'+icon_donate_paypal+'">'
			+'			</a></center>'
			+'		</div>'
			+'		<div style="display:inline; position:relative; float:left; margin-left:10px; text-align:left; vertical-align:top; width:380px;">'
			+'			Donations are for the time spent creating and updating this userscript and are greatly appreciated!'
			+'			<br>This script has been released as Open Source under the GPL.'
			+'		</div>'
			+'	</div>'
			+'	</p>'
			+'	<div style="height:20px; clear:both;"></div>'
			+'	<h2>This versions update text...</h2>'
			+'	<p style="line-height:18px;">'
			+'	'+scriptUpdateText
			+'	</p>'
			+'</div>'
			
			+'<div id="gm_tab_vidopt" class="tab_content">'
			+'	<h2>Video Title - Create a custom download file title.</h2>'
			+'	<div style="display:inline;">'
					+'	<div style="width:400px; float:left; display:inline;">'
						//+'	Title'
						+'	<span style="font-size:14px;">Title</span>'
						+'		<input id="gm_settings_vidopt_title" style="margin-top:-2px; width:400px; height:30px; font-size: 1.5em;" type="text" value="'+gm_title+'">'
					+'	</div>'
					+'	<div style="width:50px; float:left; margin-left:10px;">'
						+'	&nbsp;<br/>'
						+'		<input id="gm_settings_vidopt_titlesavebtn" style="width:auto; height:30px; font-size: 1.5em; color: #003333; border-bottom: #d3d3d3 1px solid; border-left: #d3d3d3 1px solid; border-top: #d3d3d3 1px solid; border-right: #d3d3d3 1px solid;" type="button" value="Save">'
					+'	</div>'
				+'	</div>'
			+'	<div style="clear:both;"></div>'
				+'	<br /><br /><h2>Current Title Example</h2>'
				+'	<div id="dltitle">'+title+'</div>'
			+'	<div style="clear:both;"></div>'
				+'	<br /><br /><h2>Legend (Case Sensitive)</h2>'
				+'	%T = Title of video'
				+'	<br />%U = User (uploader)'
				+'	<br />%V = Video ID'
				+'	<br />%Q = Quality (480p/720p) [NOTE - Example wont show quality, generated titles in links will.]'
				+'	<br />%L = Length (min:sec)'
				+'	<br />%Y = Year'
				+'	<br />%M = Month'
				+'	<br />%D = Day'
				+'	<br />%% = Litteral percent sign'
			+'</div>'
			
			+'<div id="gm_tab_help" class="tab_content">'
				+'	<h2>Help</h2>'
				+'	<p style="line-height:20px;">Head over to <a href="http://userscripts.org/scripts/discuss/'+scriptId+'">Discussions</a> '
				+'	and create a topic for help.<br>'
				+'	<a href="http://userscripts.org/topics/98613?page=1">Upgrade v0.4.0 instructions discussion</a>'
				+'	</p>'
			+'</div>'
			
		+'</div>'
		+'';
    GB_show(t,text,0,550);
    
    //vidopt_update_title(true);
    newtitle = $ljq("#gm_settings_vidopt_title").val();
	newtitle = newtitle.replace(/%T/g, title).replace(/%U/g, user).replace(/%Y/g, year).replace(/%M/g, month).replace(/%D/g, day).replace(/%V/g, vcode).replace(/%L/g, videoLength).replace(/%%/g, "%");
	$ljq('#gm_tab_vidopt #dltitle').html(newtitle);
    
    //$ljq("#gm_settings_vidopt_titleprefix, #gm_settings_vidopt_titlesuffix").keyup(function() {
    $ljq("#gm_settings_vidopt_title").keyup(function() {
    	if ($ljq("#gm_settings_vidopt_titlesavebtn").val() == "Saved!" ) {
    		$ljq("#gm_settings_vidopt_titlesavebtn").val("Save");
    	}
    	//vidopt_update_title(true);
    	newtitle = $ljq("#gm_settings_vidopt_title").val();
		newtitle = newtitle.replace(/%T/g, title).replace(/%U/g, user).replace(/%Y/g, year).replace(/%M/g, month).replace(/%D/g, day).replace(/%V/g, vcode).replace(/%L/g, videoLength).replace(/%%/g, "%");
		$ljq('#gm_tab_vidopt #dltitle').html(newtitle);
		//$ljq("#gm_settings_vidopt_title").animateHighlight("#dd0000", 1000);
		//$ljq("#gm_settings_vidopt_title").effect("highlight", {}, 3000);
		return false;
	});
	
	$ljq("#gm_settings_vidopt_titlesavebtn").on('click', function(event) {
		event.preventDefault();
		//alert("Save!");
		GMlog("titlesavebtn clicked");
		if ($ljq("#gm_settings_vidopt_title").val() == "") {
			alert("Cannot save a blank title!");
		} else {
			//GMlog("titlesavebtn\npre:"+$ljq("#gm_settings_vidopt_titleprefix").val()+"\nsuf:"+$ljq("#gm_settings_vidopt_titlesuffix").val());
			//GMlog("titlesavebtn:"+$ljq("#gm_settings_vidopt_title").val());
			GM_setValue('gm_title', $ljq("#gm_settings_vidopt_title").val());
			gm_title = GM_getValue('gm_title', '');
			GMlog("gm_title saved :"+gm_title);
			$ljq(this).val("Saved!");
			//vidopt_update_title(false);
			videolistRefresh = true;
		}
	});
    
    //=====================
    
    function show_confirm(msg) {
	var r=confirm(msg);
		if (r) {
			//alert("You pressed OK!");
			return true;
		} else {
 			//alert("You pressed Cancel!");
 			return false;
 		}
	}
	
    //Default Action
	$ljq("#GB_window .tab_content").hide(); //Hide all content
	$ljq("#GB_window ul.tabs li").removeClass("active").show(); //Deactivate all tabs
	$ljq("#GB_window ul.tabs li:first").addClass("active").show(); //Activate first tab
	$ljq("#GB_window .tab_content:first").show(); //Show first tab content
	
	//On Click Event
	$ljq("#GB_window ul.tabs li").on('click', function(event) {
		event.preventDefault();
		//alert("click");
		$ljq("#GB_window ul.tabs li").removeClass("active"); //Remove any "active" class
		$ljq(this).addClass("active"); //Add "active" class to selected tab
		$ljq("#GB_window .tab_content").hide(); //Hide all tab content
		var activeTab = $ljq(this).find("a").attr("href"); //Find the rel attribute value to identify the active tab + content
		$ljq(activeTab).show(); //Fade/Hide in the active content
		//alert($ljq("#GB_window").height()+"\n"+$ljq(window).height());
		//Switched tabs - check height
		tab_content_height();
		//alert(activeTab);
		return false;
	});
	
	//Initial display of content - check height
	tab_content_height();
}

function showUpgradeInstructions(browser) {
	GB_DONE = false;
	
	var t = '<span>'+scriptName+' - Upgrade process...</span>';
	var text = ''
		+'<ul class="tabs">'
		+'<li><a href="#gm_tab_firefox" class="about">Firefox</a></li>'
		+'<li><a href="#gm_tab_chrome" class="about">Chrome</a></li>'
		+'</ul>'
		+'<div class="tab_container">'
			+'<div id="gm_tab_firefox" class="tab_content">'
			+'	<h2>Firefox'
			+'	<a href="http://userscripts.org/topics/98613?page=1">Upgrade instructions discussion</a></h2>'
			+'	<span style="font-size:14px;">'
			+'	Step 1a) Click the Firefox ribbon menu, then click Add-ons.<br/>'
			+'	<img src="http://www.byronr.com/shell/greasemonkey/ytezd/v040upgradefirefox1.jpg"><br/><br/>'
			+'	Step 1b) Or click the Tools menu, then click Add-ons.<br/>'
			+'	<img src="http://www.byronr.com/shell/greasemonkey/ytezd/v040upgradefirefox2.jpg"><br/><br/>'
			+'	Step 2) Click User Scripts.<br/>'
			+'	<img src="http://www.byronr.com/shell/greasemonkey/ytezd/v040upgradefirefox3.jpg"><br/><br/>'
			+'	Step 3) Locate the old YouTube.com - EZ Download script and remove it.<br/>'
			+'	<img src="http://www.byronr.com/shell/greasemonkey/ytezd/v040upgradefirefox4.jpg"><br/><br/><br/>'
			+'	Step 4) Refresh the YouTube page and the new version will work! =D<br/><br/>'
			+'	This was caused by a minor name change in v0.4.0, sorry!'
			+'	</span>'
			+'</div>'
			
			+'<div id="gm_tab_chrome" class="tab_content">'
			+'	<h2>Chrome'
			+'	<a href="http://userscripts.org/topics/98613?page=1">Upgrade instructions discussion</a></h2>'
			+'	<span style="font-size:14px;">'
			+'	Step 1) Click the Wrench icon (top right) go to Tools, then click Extensions.<br/>'
			+'	<img src="http://www.byronr.com/shell/greasemonkey/ytezd/v040upgradechrome1.jpg"><br/><br/>'
			+'	Step 2) Locate the old YouTube.com - EZ Download script and remove it.<br/>'
			+'	<img src="http://www.byronr.com/shell/greasemonkey/ytezd/v040upgradechrome2.jpg"><br/><br/>'
			+'	Step 3) Refresh the YouTube page and the new version will work! =D<br/><br/>'
			+'	This was caused by a minor name change in v0.4.0, sorry!'
			+'	</span>'
			+'</div>'
		+'</div>'
		+'';
    GB_show(t,text,0,550);
    
    //=====================
    
    //Default Action
	$ljq("#GB_window .tab_content").hide(); //Hide all content
	$ljq("#GB_window ul.tabs li").removeClass("active").show(); //Deactivate all tabs
	//$ljq("#GB_window ul.tabs li:first").addClass("active").show(); //Activate first tab
	//$ljq("#GB_window .tab_content:first").show(); //Show first tab content
	$ljq("#GB_window ul.tabs li a[href='#gm_tab_"+browser+"']").parent().addClass("active").show(); //Activate first tab
	$ljq("#GB_window #gm_tab_"+browser+".tab_content").show(); //Show first tab content
	
	//On Click Event
	$ljq("#GB_window ul.tabs li").on('click', function(event) {
		event.preventDefault();
		//alert("click");
		$ljq("#GB_window ul.tabs li").removeClass("active"); //Remove any "active" class
		$ljq(this).addClass("active"); //Add "active" class to selected tab
		$ljq("#GB_window .tab_content").hide(); //Hide all tab content
		var activeTab = $ljq(this).find("a").attr("href"); //Find the rel attribute value to identify the active tab + content
		$ljq(activeTab).show(); //Fade/Hide in the active content
		//alert($ljq("#GB_window").height()+"\n"+$ljq(window).height());
		//Switched tabs - check height
		tab_content_height();
		//alert(activeTab);
		return false;
	});
	
	//Initial display of content - check height
	tab_content_height();
}

function tab_content_height() {
	//reset height and scroll before checking dimensions
	$ljq("#GB_window .tab_content").css({'height':'', 'overflow-y':''});
	//alert(($ljq("#GB_window").height() + 40)+"\n"+$ljq(window).height());
	//add 20px to offset top:20
	if (($ljq("#GB_window").height() + 25) > $ljq(window).height()) {
		var tab_height = $ljq(window).height() - 150;
		//alert(tab_height);
		GMlog("tab_height: "+tab_height);
		$ljq("#GB_window .tab_content").css({'height':''+tab_height+'px', 'overflow-y':'scroll'});
	//} else {
		//$ljq("#GB_window .tab_content").css({'height':'', 'overflow-y':''});
	}
}

//Vars used throughout script
var swfArgs = new Array();
//var moviePlayer = document.getElementById("movie_player");
var fmtStreamMap = false;
var videoId = '';
var title = '';
//var urltitle = ''; //set to title below
var user = '';
var date = '';
//title = $ljq.trim($ljq("#watch-headline-title").text());
//title = $ljq.trim($ljq("#eow-title").attr("title"));
//title = $ljq.trim($ljq("#watch-headline-title").text());
//<h1 id="watch-headline-title"><span  class=" yt-uix-expander-head" dir="ltr" title="Video Title">
//urltitle = title;
//alert(title);
//user = $ljq("a#watch-userbanner img").attr("alt");
//user = $ljq("#watch-headline-user-info a#watch-userbanner").attr("title");
//user = $ljq("#watch-username").text();
//if (user == '' || user == null) {
//	user = $ljq("#watch-userbanner").attr("title");	
//	if (user == '' || user == null) {
//	}
//}
//alert($ljq("a[rel='author']:first").text());
/*
user = $ljq.trim($ljq("a[rel='author']:first").text());
if (user == '' || user == null || user == undefined) {
	//alert("user:first null");
	user = $ljq.trim($ljq("a[rel='author']:first").attr('title'));
}
if (user == '' || user == null || user == undefined) {
	//alert("user null");
	user = $ljq.trim($ljq("a[rel='author']:last").text())
}
if (user == '' || user == null || user == undefined) {
	user = '';
}
*/
//user = $ljq.trim($ljq("span.yt-user-name:first").text());
user = $ljq.trim($ljq("a.yt-user-name:first").text());
//check updated source
//$ljq("a.yt-user-name:first").attr('data-text', 'USERNAME-HERE');
//alert(user);
//date = $ljq.trim($ljq("span#eow-date-short").text());
try {
	date = $ljq.trim($ljq("#eow-date").text());
	//date = date.replace(/,/ig, '').replace(/January/ig, '01').replace(/February/ig, '02').replace(/March/ig, '03').replace(/April/ig, '04').replace(/May/ig, '05').replace(/June/ig, '06').replace(/July/ig, '07').replace(/August/ig, '08').replace(/September/ig, '09').replace(/October/ig, '10').replace(/November/ig, '11').replace(/December/ig, '12');
	date = date.replace(/,+/ig, '').replace(/\s+/ig, ' ').replace(/Jan/ig, '01').replace(/Feb/ig, '02').replace(/Mar/ig, '03').replace(/Apr/ig, '04').replace(/May/ig, '05').replace(/Jun/ig, '06').replace(/Jul/ig, '07').replace(/Aug/ig, '08').replace(/Sep/ig, '09').replace(/Oct/ig, '10').replace(/Nov/ig, '11').replace(/Dec/ig, '12');
	date = date.split(" ");
	//date[month] = date[0];
	//alert(date[month]);
	//alert(date.length);
	var year = date[2];
	var month = date[0];
	var day = date[1];
	if (month.length < 2) {
		month = "0"+month;
	}
	if (day.length < 2) {
		day = "0"+day;
	}
//alert("date:"+date+"\n"+"year:"+year+"\n"+"month:"+month+"\n"+"day:"+day);
} catch(e) {}

/*
var mydate = new Date();
var year = mydate.getYear() + 1900;
//var day = mydate.getDay();
var month = mydate.getMonth();
var day = mydate.getDate()
if (day<10) { day="0"+day }
//alert(day);
*/

var curwindow = window.location.toString();
var vcode = "";
//alert(curwindow);
//pattern = /(https?:\/\/.*\?v=(.*)[^&].*).*/ig;
//pattern = /(https?:\/\/.*\?v=([a-zA-Z0-9-_]+))/ig;
//try {
//	pattern = /v=([a-zA-Z0-9-_]+)/ig;
//	matches = curwindow.match(pattern);
//	//alert(matches);
//	//alert(matches[0].replace(pattern, "$2"));
//	var vcode = matches[0].replace(pattern, "$1");
//} catch(e) {}
//siteuri = siteuri.split("/");
//alert(siteuri[0]);
//siteuri = siteuri[0];
//siteuri = siteuri.replace('www.', '');
//alert(siteuri);
pattern = "";
matches = "";

var duplicatecheck = false;
if ($ljq("#gmmnu").length > 0) {
	duplicatecheck = true;
	$ljq("#gmmnu").remove();
}
//duplicatecheck = true;
//alert(duplicatecheck);

//if (duplicatecheck == true) {
//GMlog("Append menu SKIPPED");
//} else {
GMlog("Append menu");
//Show menu as soon as possible
$ljq(document.body).append("<div id='gmmnu'></div>");
$ljq("#gmmnu").html(""
		+"\n<div id='gmmnu_title' data-title='YouTube Download'>YouTube Download</div>\n"
		+"<div id='gmmnu_menu'>\n"
		//+"<div class='header'>Menu</div>\n"
		//+"<div>1st | 2nd</div>\n"
			+"<div class='menubar'>\n"
				//+"<a href='#config' title='Config'><div class='config'></div></a>\n"
				//+"<a href='#config' class='config' title='Config'>&nbsp;</a>\n"
				+"<div id='config_icon'><a href='#config' class='config' data-title='Configuration'></a></div>\n"
				//+"<a href='#update' class='update' title='Update'></a>\n"
				+"<div id='update_icon'>"
					+"<a href='#update' class='update_grey' data-title='No Updates'></a>"
				+"</div>\n"
				/*
				//watched
				+"<div id='viewed_icon'>"
					//+"<a href='#viewed' class='viewed_grey' data-title='Viewed'></a>"
					//+"<a href='#viewed' class='viewed' data-title='Viewed'></a>"
					+"<a href='#viewed' class='viewed' data-title='Viewed'></a>"
				+"</div>\n"
				+"<div id='starred_icon'>"
					//+"<a href='#starred' class='starred_grey' data-title='Favorite'></a>"
					+"<a href='#starred' class='starred' data-title='Favorite'></a>"
				+"</div>\n"
				*/
				//+"<div id='donate_icon'><a href='"+donationLink+"' class='donate' data-title='Donate! =D' target='_blank'></a></div>\n"
				+"<div id='home_icon'><a href='http://userscripts.org/scripts/show/"+scriptId+"' class='home' data-title='Script Homepage' target='_blank'></a></div>\n"
				+"<div id='twitter_icon'><a href='http://twitter.com/Daem0nX' class='twitter' data-title='Follow Us' target='_blank'></a></div>\n"
				+"<div id='donate_icon'><a href='#donations' class='donate' data-title='Donate! =D'></a></div>\n"
			+"</div>\n"
		+"</div>\n"
		+"<div id='gmmnu_links_wrap'>\n"
		+"<div id='gmmnu_links'>\n"
			//+"<div id='gmmnulinksheader'>Files</div>\n"
			+"<div class='header'>Files"
				//+"<div class='format_mp4'>MP4</div>\n"
				//+"<div class='format_flv'>FLV</div>\n"
				//+"<div class='format_webm'>WebM</div>\n"
			+"</div>\n"
			+"<div id='gmmnu_linksfiles'>"
			+"	<div class='status'>Searching...</div>"
			+"</div>\n"
		+"</div>\n"
		+"<div id='gmmnu_links_3d'>\n"
			+"<div class='header'>3D Files</div>\n"
			+"<div id='gmmnu_linksfiles_3d'>"
			+"	<div class='status'>Searching...</div>"
			+"</div>\n"
		+"</div>\n"
		+"</div>\n"
		+"<div id='gmmnu_err'>\n"
			//+"<div id='gmmnuerrheader'>Error Alert</div>\n"
			+"<div class='header'>Error Alert</div>\n"
			//id='gmmnuheader'
			+"<div id='gmmnu_errmsg'>There was an error and the files couldn't be loaded.<br>"
			+"Please copy the URL (address) and report the problem + URL to <strong><a href='http://userscripts.org/scripts/discuss/"+scriptId+"'>Discussions</a></strong>"
			+"</div>\n"
		+"</div>\n"
		+"<div id='gmmnu_donations' style='font-size:12px;'>\n"
			+"<div class='header'>Donation Methods</div>\n"
			+"Please consider donating, its greatly appreciated!\n"
			+"<center><a href='"+donationLink_flattr+"' class='icon_donate_flattr' target='_blank'></a></center>\n"
			+"<center><a href='"+donationLink_paypal+"' class='icon_donate_paypal' target='_blank'></a></center>\n"
		+"</div>\n"
		+"");
GMlog("Append menu : Success");
//}

if (duplicatecheck == true) {
	$ljq("#gmmnu").append(""
	+"	<div style='background: url(http://www.byronr.com/shell/greasemonkey/ytezd/v040upgradefirefox1.jpg) no-repeat -9999px -9999px'></div>\n"
	+"	<div style='background: url(http://www.byronr.com/shell/greasemonkey/ytezd/v040upgradefirefox2.jpg) no-repeat -9999px -9999px'></div>\n"
	+"	<div style='background: url(http://www.byronr.com/shell/greasemonkey/ytezd/v040upgradefirefox3.jpg) no-repeat -9999px -9999px'></div>\n"
	+"	<div style='background: url(http://www.byronr.com/shell/greasemonkey/ytezd/v040upgradefirefox4.jpg) no-repeat -9999px -9999px'></div>\n"
	+"	<div style='background: url(http://www.byronr.com/shell/greasemonkey/ytezd/v040upgradechrome1.jpg) no-repeat -9999px -9999px'></div>\n"
	+"	<div style='background: url(http://www.byronr.com/shell/greasemonkey/ytezd/v040upgradechrome2.jpg) no-repeat -9999px -9999px'></div>\n"
	+"")
	GMlog("Append preload images");
}
		
function showError() {
	//Hide SD and HD
	$ljq("#gmmnu_links, #gmmnu_links_3d").hide();
	//Show Err
	$ljq("#gmmnu_err").show();
	$ljq("#gmmnu_err").attr("data-active", "true");
}

var formats = {
	5:  { itag: 5 , quality: 5, description: "Low Quality, 240p"     		, format: "FLV" , fres: "240p", 	mres: { width:  400, height:  240 }, acodec: "MP3"   , vcodec: "SVQ"   },
	17: { itag: 17, quality: 4, description: "Low Quality, 144p"			, format: "3GP" , fres: "144p", 	mres: { width:  0, height: 0  }, acodec: "AAC"   , vcodec: "" },
	18: { itag: 18, quality: 15, description: "Low Definition, 360p"		, format: "MP4" , fres: "360p", 	mres: { width:  480, height:  360 }, acodec: "AAC"   , vcodec: "H.264" },
	22: { itag: 22, quality: 35, description: "High Definition, 720p" 		, format: "MP4" , fres: "720p", 	mres: { width: 1280, height:  720 }, acodec: "AAC"   , vcodec: "H.264" },
	34: { itag: 34, quality: 10, description: "Low Definition, 360p"  		, format: "FLV" , fres: "360p", 	mres: { width:  640, height:  360 }, acodec: "AAC"   , vcodec: "H.264" },
	35: { itag: 35, quality: 25, description: "Standard Definition, 480p"  	, format: "FLV" , fres: "480p", 	mres: { width:  854, height:  480 }, acodec: "AAC"   , vcodec: "H.264" },
	36: { itag: 36, quality: 6, description: "Low Quality, 240p"  			, format: "3GP" , fres: "240p", 	mres: { width:  0, height:  0 }, acodec: "AAC"   , vcodec: "" },
	37: { itag: 37, quality: 45, description: "Full High Definition, 1080p"	, format: "MP4" , fres: "1080p", 	mres: { width: 1920, height: 1080 }, acodec: "AAC"   , vcodec: "H.264" },
	38: { itag: 38, quality: 55, description: "Original Definition" 		, format: "MP4" , fres: "Orig", 	mres: { width: 4096, height: 3072 }, acodec: "AAC"   , vcodec: "H.264" },
	43: { itag: 43, quality: 20, description: "Low Definition, 360p"  		, format: "WebM", fres: "360p", 	mres: { width:  640, height:  360 }, acodec: "Vorbis", vcodec: "VP8"   },
	44: { itag: 44, quality: 30, description: "Standard Definition, 480p"	, format: "WebM", fres: "480p", 	mres: { width:  854, height:  480 }, acodec: "Vorbis", vcodec: "VP8"   },
	45: { itag: 45, quality: 40, description: "High Definition, 720p" 		, format: "WebM", fres: "720p", 	mres: { width: 1280, height:  720 }, acodec: "Vorbis", vcodec: "VP8"   },
	46: { itag: 46, quality: 50, description: "Full High Definition, 1080p"	, format: "WebM", fres: "1080p", 	mres: { width: 1280, height:  720 }, acodec: "Vorbis", vcodec: "VP8"   },
	82: { itag: 82, quality: 16, description: "Low Definition 3D, 360p"		, format: "MP4",  fres: "360p",		mres: { width: 640,  height:  360 }, acodec: "AAC"	 , vcodec: "H.264" },
	84: { itag: 84, quality: 41, description: "High Definition 3D, 720p"	, format: "MP4",  fres: "720p",		mres: { width: 1280, height:  720 }, acodec: "AAC"	 , vcodec: "H.264" },
	100: { itag: 100, quality: 17, description: "Low Definition 3D, 360p"	, format: "WebM", fres: "360p", 	mres: { width: 640,  height:  360 }, acodec: "Vorbis", vcodec: "VP8"   },
	102: { itag: 102, quality: 42, description: "High Definition 3D, 720p"	, format: "WebM", fres: "720p", 	mres: { width: 1280, height:  720 }, acodec: "Vorbis", vcodec: "VP8"   },
	}
var orderedformats = [];

//$ljq(document).ready(function(){
	//alert("ljq");
		
	if (duplicatecheck == true) {
		$ljq("#GB_initialize").remove();
	}
	
	//Append the greasebox to the body of the site
	$ljq(document.body).append("<div id='GB_initialize'></div>");
	
	if (debugSettings == true) {
		showConfiguration();
	}
	//showUpgradeInstructions();
	//showError();
	//showSettings();
	var zztxt = '';
	var minVar = 0;
	var secVar = 0;
	
	GMlog("title: "+title);
	GMlog("Get flashvars");
	
	try {
		//alert($ljq("script:contains('yt.playerConfig')").html());
		//var streamMap = $ljq("script:contains('yt.playerConfig')").html();
		var streamMap = $ljq("script:contains('ytplayer.config')").html();
		//GMlog(streamMap);
		
		//parse yt.playerConfig for flashvars
		streamMap = streamMap.match(/^.*((\r\n|\n|\r)|$)/gm);
		for (var i = 0; i < streamMap.length; i ++)	{
			//if (streamMap[i].indexOf("playerConfig") != -1) {
			if (streamMap[i].indexOf("args") != -1) {
				//GMlog("streamMap: "+streamMap[i]);
				streamMap = streamMap[i];
				break;
			}
		}
		//streamMap = streamMap.replace(/yt.playerConfig\s?=\s?(.*)\;/ig, "$1");
		//streamMap = streamMap.replace(/ytplayer.config\s?=\s?(.*)\;/ig, "$1");
		streamMap = streamMap.replace(/.*?ytplayer\.config\s?=\s?(\{.*?\});.*?/ig, "$1");
		GMlog(streamMap);
		streamMap = JSON.parse(streamMap);
		//GMlog(streamMap.args.video_id);
		vcode = streamMap.args.video_id;
		//GMlog(streamMap.args.url_encoded_fmt_stream_map);
		//GMlog(streamMap.args.length_seconds);
		videoLength = streamMap.args.length_seconds;
		title = streamMap.args.title;
		//zztmp = zztmp.args.url_encoded_fmt_stream_map;

		//pattern = /.*?playerConfig = (\{.*?\})/ig;
		//matches = zztmp.match(pattern, "$1");
		//GMlog(zztmp);
		//alert(zztmp);
		//alert(decodeURI(zztmp));
		
		//
		//pattern = /.*?length_seconds":\s?([0-9]+).*/ig;
		//matches = zztmp.match(pattern);
		//alert(matches[0].replace(pattern, "$1"));
		//videoLength = matches[0].replace(pattern, "$1");
		//GMlog(videoLength);
		//

		minVar = Math.floor(videoLength/60);  // The minutes
		secVar = videoLength % 60;            // The balance of seconds
		if (secVar < 10) {
			secVar = "0"+secVar;
		}
		videoLength = minVar+":"+secVar;
		//GMlog(videoLength);
		//alert(videoLength);
	
		//pattern = /.*?url_encoded_fmt_stream_map":\s?"(url=.*?)".*/ig;
		//pattern = /.*?url_encoded_fmt_stream_map":\s?"(.*?)".*/ig;
		//matches = zztmp.match(pattern);
		//title = unescape(matches[0].replace(pattern, "$2"));
		//zztmp = matches[0].replace(pattern, "$1").replace("url=", "");
		
		//GMlog(zztmp);
		//alert(zztmp);
		//ztmp = ztmp.replace(/'u0026'/ig, '&');
		//alert(ztmp);
		//alert(unescape(ztmp));
		//alert(decodeURI(ztmp));
		//alert(decodeURI(unescape(ztmp)));
		//ztmp = ztmp.replace("\\u0026", "&").replace("\\", "");
		//alert(unescape(decodeURI(ztmp)));
		
		//ztmp = ztmp.split(",");
		//for (var i = 0; i < ztmp.length; i ++)	{
			//if (i <= 0) {
				//alert(unescape(decodeURI(ztmp[i])));
			//}
		//}
		//ztmp = ztmp.split("&");
		//GMlog("split streamMap length: "+split.length);
		
		//streamMap = ztmp;
		//streamMap = zztmp;
		//streamMap = unescape(zztmp);
		//GMlog(streamMap);
		//zztmp = "";
		//alert(streamMap);
		
		var url = "";
		var itag = 0;
		var name = "";
		var zznum = 0;

		streamMap = streamMap.args.url_encoded_fmt_stream_map;
		streamMap = streamMap.split(",");
		for (var i = 0; i < streamMap.length; i ++)	{
			GMlog("streamMap.length = "+streamMap.length);
			GMlog("streamMap.length = "+i);
			//GMlog(streamMap[i]);
			//GMlog(unescape(streamMap[i]));
			//GMlog(unescape(unescape(streamMap[i])));
			//alert(unescape(streamMap[i]));
			//pattern = /(.*?)(\\|\+)/ig;
						
			//pattern = /.*?(http.*)/ig;
			//matches = streamMap[i].match(pattern);
			//title = unescape(matches[0].replace(pattern, "$2"));
			//ztmp = matches[0].replace(pattern, "$1");
			//url = matches[0].replace(pattern, "$1").replace("sig=", "signature=");
			//url = decodeURIComponent(url);
			//url = decodeURI(url);
			//url = escape(url);
			//url = streamMap[i];
			//url !== "" && 

			parseLink(streamMap[i], zznum);
			GMlog("call parseLink = "+zznum);
			zznum = zznum + 1;
			
			//url = decodeURIComponent(url);

			/*
			*
			* FIREBUG causing issue when enabled
			* Signature will not be in URL
			*
			*/			
		}
	} catch(ex) {
		showError();
	}
	pattern = '';
	matches = '';
	streamMap = null;
	
	function parseLink(url, num){
	/*
	* Check for expected params and rebuild link
	* http entry - required (ex http://123.a-z.youtube.com/videoplayback?key=yt1)
	* fallback_host - required
	* itag - required (exclude duplicate)
	* sig(nature) - required
	* sver - required
	* sparams - required
	* cp - required
	* upn - required
	* ms - required
	* ipbits - required
	* gcr - required
	* ratebypass - required
	* source - required
	* mv - required
	* expire - required
	* ip - required
	* newshard - required
	* mt - required
	* fexp - required
	* quality - not needed
	* type - not needed
	*/

	var zztmp = "";
	GMlog("base url = "+url);
	//signature = url.replace("/.*?sig=([0-9a-z\.]+).*", "$1");
	//GMlog("signature = "+signature);
	//url = decodeURIComponent(url).replace(/.*?(http.*)/ig, "$1").replace(/sig=[0-9a-z\.]+/ig, "");
	url = decodeURIComponent(url);
	url = url.replace("url=", "").replace("sig=", "signature=");;
		if (url.indexOf("http") != -1) {
			GMlog("decode url = "+url);
			//explode for all query variables
			//rebuild url
			zztmp = url.split("&");
			url = "";
			for (var j = 0; j < zztmp.length; j ++)	{
				GMlog(zztmp[i]);
				if (/^(type)/.test(zztmp[j])) {
					GMlog("skip");
				} else {
					if (/^(http)/.test(zztmp[j])) {
						url = zztmp[j] + url;
					} else {
						url = url + "&" + zztmp[j];
					}
					//GMlog("added = "+zztmp[i]);
				}
				//if (zztmp.indexOf("quality") != -1 && zztmp.indexOf("type") != -1){
				//if ()
				//	GMlog(zztmp[i]);

				//}
			}
			//GMlog("end");
			//GMlog(url);
			//break;

			//url = url.replace(/.*?(http.*)/ig, "$1");
			//GMlog();
			
			pattern = /itag=([0-9]+)/ig;
			matches = url.match(pattern);
			itag = matches[0].replace(pattern, "$1");
			orderedformats[num] = {};
			orderedformats[num]["itag"] = itag;
			//orderedformats[zznum]["url"] = unescape(url.replace("url=", ""));
			//orderedformats[zznum]["url"] = unescape(url.replace("url=", ""));
			//url = url.replace("url=", "");
			url = encodeURI(url);
			//url = url.replace(/%5Cu0026/ig, "&amp;");
			url = url.replace(/%5Cu0026/ig, "&");
			url = url.replace(/%252C/ig, ",");
			url = url.replace(/\&itag=[0-9]+/ig, "");
			url = url + "&itag=" + itag;
			//&type=video/mp4;+codecs=%22avc1.64001F,+mp4a.40.2%22
			//url = url.replace(/type=video.*/ig, "--video--");
			
			GMlog("url = "+url);
			//alert(url);
			//url = decodeURIComponent(url);
			orderedformats[num]["url"] = url;
			//GMlog("url = "+unescape(url.replace("url=", "")));
			//GMlog("url = "+escape(url.replace("url=", "")));
			//GMlog("url = "+url);
			try {
				orderedformats[num]["quality"] = formats[itag].quality;
			} catch(e) {
				orderedformats[num]["quality"] = 0;
			}
			GMlog("parseLink = "+num);
		}
	}

	//if ($ljq("#watch-player.flash-player").length == 0 && $ljq("#watch-player.html5-player").length == 0) {
	//	$ljq("#gmmnu_err #gmmnu_errmsg").html("No Flash or HTML5 player detected!");
	//	showError();
	//}
	
	//GMlog("orderedformats length = "+orderedformats.length);
	//GMlog(orderedformats);
	var fmts = "";
	//var url = "";
	
	//zztxt = "";
	//for (var i = 0; i < orderedformats.length; i ++) {
	//	zztxt += ","+orderedformats[i]["itag"];
	//}
	//GMlog("presort: "+zztxt);
	
	//if (orderedformats.length > 0) {
		orderedformats.sort(function(a,b) {
			// assuming distance is always a valid integer
			//GMlog(parseInt(a.quality)+" - "+parseInt(b.quality));
			return parseInt(a.quality) - parseInt(b.quality);
		});
	//}
	//orderedformats.reverse();
	//zztxt = "";
	//for (var i = 0; i < orderedformats.length; i ++) {
		//zztxt += ","+orderedformats[i]["itag"];
		//zztxt += ","+orderedformats[i]["quality"];
	//}
	//GMlog("postsort: "+zztxt);
	for (i = 1; i < orderedformats.length; i++ ) {
		//GMlog("look: "+orderedformats[i]["itag"]);
    	if ( orderedformats[i]["itag"] === orderedformats[ i - 1 ]["itag"] ) {
    		//GMlog("splice: "+orderedformats[i]["itag"]);
	        orderedformats.splice( i--, 1 );
	    }
	}
	
	function generate_links() {	
	//alert("generate_links");
	try {
	newtitle = newtitle.replace(/%T/g, title).replace(/%U/g, user).replace(/%Y/g, year).replace(/%M/g, month).replace(/%D/g, day).replace(/%V/g, vcode).replace(/%L/g, videoLength).replace(/%%/g, "%");
	newtitle = newtitle.replace(/^\s\s*/, "").replace(/\s\s*$/, "").replace(/"/g, "-").replace(/=/g, "%3D").replace(/,/g, "%2C").replace(/&/g, "%26").replace(/#/g, "%23").replace(/\?/g, "%3F").replace(/\//g, "_").replace(/\\/g, "_").replace(/ /g, "+");
	//var urltitle = title.replace(/^\s\s*/, "").replace(/\s\s*$/, "").replace(/"/g, "-").replace(/%/g, "%25").replace(/=/g, "%3D").replace(/,/g, "%2C").replace(/&/g, "%26").replace(/#/g, "%23").replace(/\?/g, "%3F").replace(/\//g, "_").replace(/\\/g, "_").replace(/ /g, "+");
	//var urltitle = newtitle;
	//GMlog("urltitle: "+urltitle);
	var itag = 0;
	$ljq("#gmmnu_linksfiles, #gmmnu_linksfiles_3d").html('');
	for (var i = 0; i < orderedformats.length; i ++) {
		fmts = "";
		itag = orderedformats[i]["itag"];
		var qltytitle = '';
		//var qltytext = (formats[itag].fres !== '' ? formats[itag].fres : "");
		//(formats[itag].fres !== '' ? formats[itag].fres : "")
		try {
			qltytext = formats[itag].fres;
			//GMlog("fres: "+formats[itag].fres);
		} catch(e) {
			qltytext = "fmt"+itag;
			//GMlog("fres error: "+itag);
		}
		//try {
			//qltytitle = newtitle.replace(/%Q/g, formats[itag].fres).replace(/^\s\s*/, "").replace(/\s\s*$/, "").replace(/"/g, "-").replace(/%/g, "%25").replace(/=/g, "%3D").replace(/,/g, "%2C").replace(/&/g, "%26").replace(/#/g, "%23").replace(/\?/g, "%3F").replace(/\//g, "_").replace(/\\/g, "_").replace(/ /g, "+");
			//GMlog("fres : "+formats[itag].fres);
			//GMlog("urltitle : "+urltitle);
		//} catch(e) {
			//qltytitle = newtitle.replace(/^\s\s*/, "").replace(/\s\s*$/, "").replace(/"/g, "-").replace(/%/g, "%25").replace(/=/g, "%3D").replace(/,/g, "%2C").replace(/&/g, "%26").replace(/#/g, "%23").replace(/\?/g, "%3F").replace(/\//g, "_").replace(/\\/g, "_").replace(/ /g, "+");
		//}
		//qltytitle = newtitle.replace(/%Q/g, qltytext).replace(/^\s\s*/, "").replace(/\s\s*$/, "").replace(/"/g, "-").replace(/%/g, "%25").replace(/=/g, "%3D").replace(/,/g, "%2C").replace(/&/g, "%26").replace(/#/g, "%23").replace(/\?/g, "%3F").replace(/\//g, "_").replace(/\\/g, "_").replace(/ /g, "+");
		qltytitle = newtitle.replace(/%Q/g, qltytext).replace(/%/g, "%25");
		GMlog("urltitle: "+qltytitle);
		try {
			GMlog("itag: "+itag+" | quality: "+formats[itag].quality);
			//fmts = "\n<div class=\"link\"><a href=\"" + orderedformats[i]["url"] + "&title="+title+ "\" data-title=\""+formats[itag].format+", "+formats[itag].vcodec+"/"+formats[itag].acodec+"\" data-title-default=\""+formats[itag].description+"\" data-format=\""+formats[itag].format.toLowerCase()+"\" data-quality=\""+formats[itag].quality+"\" data-formatid=\""+itag+"\" class=\"\">"+formats[itag].description+"<span class=\"format\">"+formats[itag].format+"</span></a></div>";
			//fmts = "\n<div class=\"link\"><a href=\"" + orderedformats[i]["url"] + "&title="+urltitle+ "\" data-format=\""+formats[itag].format.toLowerCase()+"\" data-quality=\""+formats[itag].quality+"\" data-formatid=\""+itag+"\" class=\"\">"+formats[itag].description+"<span class=\"format\">"+formats[itag].format+"</span></a></div>";
			//fmts = "\n<div class=\"link\"><div class=\"side_size\">100MB</div><a href=\"" + orderedformats[i]["url"] + "&title="+qltytitle+ "\" data-format=\""+formats[itag].format.toLowerCase()+"\" data-quality=\""+formats[itag].quality+"\" data-formatid=\""+itag+"\" class=\"\" "+(isWebkit ? "target=\"_blank\"" : "")+">"+formats[itag].description+"<span class=\"format\">"+formats[itag].format+"</span></a></div>";
			//works//fmts = "\n<div class=\"link\"><a href=\"" + orderedformats[i]["url"] + "&title="+qltytitle+ "\" data-format=\""+formats[itag].format.toLowerCase()+"\" data-quality=\""+formats[itag].quality+"\" data-formatid=\""+itag+"\" class=\"\" "+(isWebkit ? "target=\"_blank\"" : "")+">"+formats[itag].description+"<span class=\"format\">"+formats[itag].format+"</span></a></div>";
			fmts = "\n<div class=\"link\"><a href=\"" + orderedformats[i]["url"] + "&title="+qltytitle+ "\" data-format=\""+formats[itag].format.toLowerCase()+"\" data-quality=\""+formats[itag].quality+"\" data-formatid=\""+itag+"\" class=\"\" target=\"_blank\">"+formats[itag].description+"<span class=\"format\">"+formats[itag].format+"</span></a></div>";
			//works ff, chrome restarts vid //fmts = "\n<div class=\"link\"><a href=\"" + orderedformats[i]["url"] + "&title="+qltytitle+ "\" data-format=\""+formats[itag].format.toLowerCase()+"\" data-quality=\""+formats[itag].quality+"\" data-formatid=\""+itag+"\" class=\"\">"+formats[itag].description+"<span class=\"format\">"+formats[itag].format+"</span></a></div>";
			//fmts = "\n<div class=\"link\"><a href=\"" + orderedformats[i]["url"] + "&itag="+ itag + "&title="+qltytitle+ "\" data-format=\""+formats[itag].format.toLowerCase()+"\" data-quality=\""+formats[itag].quality+"\" data-formatid=\""+itag+"\" class=\"\" "+(isWebkit ? "target=\"_blank\"" : "")+">"+formats[itag].description+"<span class=\"format\">"+formats[itag].format+"</span></a></div>";
			if (formats[itag].description.indexOf("3D") != -1) {
				$ljq("#gmmnu_linksfiles_3d").prepend(fmts);
			} else {
				//if ($ljq("#gmmnu_linksfiles .link a[data-quality='"+formats[itag].quality+"']").length > 0) {
					//skip duplicate
				//} else {
					$ljq("#gmmnu_linksfiles").prepend(fmts);
				//}
			}
		} catch(e) {
			//Unknown format - append to links
			GMlog("Unknown itag: "+itag);
			//fmts = "\n<div class=\"link\"><a href=\"" + orderedformats[i]["url"] + "&title="+urltitle+ "\" class=\"\">Unknown Format<span class=\"format\">"+itag+"</span></a></div>";
			//works//fmts = "\n<div class=\"link\"><a href=\"" + orderedformats[i]["url"] + "&title="+qltytitle+ "\" class=\"\" "+(isWebkit ? "target=\"_blank\"" : "")+">Unknown Format<span class=\"format\">"+itag+"</span></a></div>";
			fmts = "\n<div class=\"link\"><a href=\"" + orderedformats[i]["url"] + "&title="+qltytitle+ "\" class=\"\" target=\"_blank\">Unknown Format<span class=\"format\">"+itag+"</span></a></div>";
			//works ff, chrome restarts vid //fmts = "\n<div class=\"link\"><a href=\"" + orderedformats[i]["url"] + "&title="+qltytitle+ "\" class=\"\">Unknown Format<span class=\"format\">"+itag+"</span></a></div>";
			//fmts = "\n<div class=\"link\"><a href=\"" + orderedformats[i]["url"] + "&itag="+ itag + "&title="+qltytitle+ "\" class=\"\" "+(isWebkit ? "target=\"_blank\"" : "")+">Unknown Format<span class=\"format\">"+itag+"</span></a></div>";
			$ljq("#gmmnu_linksfiles").append(fmts);
		}
	}
	fmts = "";
	if ($ljq("#gmmnu_linksfiles_3d a").length != 0) {
		//$ljq("#gmmnu_links_3d").css({display: "block"});
		$ljq("#gmmnu_links_3d").show();
	}
	} catch(e) {
		showError();
	}
	//alert($ljq.trim($ljq("#gmmnu_linksfiles").text()));
	//if ($ljq.trim($ljq("#gmmnu_linksfiles").text().length == 0) || ($ljq.trim($ljq("#gmmnu_linksfiles").text().length == 0) && $ljq.trim($ljq("#gmmnu_linksfiles_3d").text().length == 0))) {
	//	showError();
	//}
	} //generate_links()
	//vidopt_update_title(false);
	newtitle = gm_title;
	generate_links();
	//alert($ljq("#gmmnu_linksfiles_3d a").length);
	
	//Replace flash with custom attr in Chrome
	if (isWebkit) {
		if ($ljq("#watch-player.flash-player").length > 0) {
			GMlog("webkit : movie_player hack");
			$ljq("#movie_player").attr('wmode', 'opaque');
			//$ljq("#movie_player").attr('wmode', 'transparent');
			//$ljq("#movie_player").css('z-index', '100');
			//alert($ljq("#movie_player").attr("wmode"));
			var embededflash = $ljq("#watch-player").html();
			//alert(embededflash);
			//$ljq("#watch_player").remove();
			$ljq("#watch-player").html(embededflash);
		}
	}
	
	if (duplicatecheck == true) {
		$ljq("#gmmnu_err #gmmnu_errmsg").html('You have 2 versions of YouTube EZ Download installed! <span id="gm_upgrade_instructions"><a href="#gm_upgrade_instructions" style="font-size:12px;text-decoration:underline;">Click here for easy upgrade instructions.</a></span>');
		showError();
	}
	
	/*
	$ljq("#gmmnu_linksfiles .link a").hover(
		function() {
			//$ljq(this).attr("data-title-default", $ljq(this).text());
			$ljq(this).text($ljq(this).attr("data-title")).addClass("format");
		},
		function () {
			$ljq(this).text($ljq(this).attr("data-title-default")).removeClass("format");
		}
	);
	*/

	$ljq("#gmmnu_linksfiles .link a").on('click', function(event) {
		event.preventDefault();
		var checkurl = $ljq(this).attr('href');
		getFile(checkurl);
	});

	//Direct browser to file location
	function getFile(url) {
		GMlog("getFile: "+url);
		window.location = url;
	}
	
	//$ljq("#gmmnu_menu a").on('click', function(event) {
	//	alert($ljq(this).parent().html());
	//});
	
	$ljq("#gm_upgrade_instructions a").on('click', function(event) {
		event.preventDefault();
		if (isWebkit) {
			showUpgradeInstructions('chrome');
		} else {
			showUpgradeInstructions('firefox');
		}
		return false;
	});
	
	//span[id='playlist-bar-controls'] 
	$ljq("#playlist-bar-toggle-button").on("click", function(event) {
		playlistbarstatus("true");
	});
	
	$ljq("#gmmnu_menu .menubar div a").hover(
		function () {
			//$ljq("#gmmnu_title").html('Config');
			$ljq("#gmmnu #gmmnu_title").html($ljq(this).attr("data-title"));
		},
		function () {
			//$ljq("#gmmnu #gmmnu_title").html('YouTube Download');
			$ljq("#gmmnu #gmmnu_title").html($ljq("#gmmnu #gmmnu_title").attr("data-title"));
		}
	);
	
	$ljq("#gmmnu_menu #donate_icon").on('click', function(event) {
		event.preventDefault();
		return false;
	});
	
	//$ljq("#gmmnu_menu #donate_icon").on('click', function(event) {
	$ljq("#gmmnu_menu #donate_icon, #gmmnu_donations").hover(
		function() {
		//alert($ljq("#gmmnu_links_wrap").css("height"));
			//var newheight = $ljq("#gmmnu_links_wrap").css("height");
			var newheight = $ljq("#gmmnu_links_wrap").height();
			GMlog("newheight : "+newheight);
			//GMlog("gmmenu gmmnu_links_wrap: "+$ljq("#gmmnu #gmmnu_links_wrap").height());
			//if (newheight.replace("px", "") < 100) {
			if (newheight < 100) {
				newheight = 140;
			}
			//Is showError active?
			if ($ljq("#gmmnu_err").attr("data-active")) {
				//GMlog("data-active : "+$ljq("#gmmnu_err").attr("data-active"));
				$ljq("#gmmnu_err").hide();
			} // else {
				$ljq("#gmmnu_donations").css({height: newheight});
				$ljq("#gmmnu_links_wrap").hide();
				$ljq("#gmmnu_donations").show();
				$ljq("#gmmnu #gmmnu_title").html($ljq("#gmmnu_menu #donate_icon a").attr("data-title"));
			//}
		},
		function () {
			if ($ljq("#gmmnu_err").attr("data-active")) {
				$ljq("#gmmnu_err").show();
			} //else {
				$ljq("#gmmnu_donations").hide();
				$ljq("#gmmnu_links_wrap").show();
				$ljq("#gmmnu #gmmnu_title").html($ljq("#gmmnu #gmmnu_title").attr("data-title"));
			//}
		}
	);
	
	$ljq('#gmmnu_menu .menubar #config_icon a').on('click', function(event) {
		event.preventDefault();
		//alert("Configure!");
		showConfiguration();
		return false;
	});
	
	$ljq("#gmmnu_menu .menubar #update_icon a").on('click', function(event) {
		//if (!isWebkit) {
			event.preventDefault();
			if ($ljq("#gmmnu_menu .menubar #update_icon a").hasClass('update')) {
				alertUpdate(scriptId, name, ver, notes);
			}
		//}
		return false;
	});
	
	$ljq('#gb_update_wait').on('click', function(event) {
		event.preventDefault();
		//alert(lastCheck);
		GM_setValue('lastCheck', currentTime);
		GB_hide();
		return false;
	});
	
	$ljq('#gb_update_waitnextversion').on('click', function(event) {
		event.preventDefault();
		//return false;
		//alert(latest_version.major+'.'+latest_version.minor+'.'+latest_version.patch);
		GM_setValue('lastVersion', latest_version.major+'.'+latest_version.minor+'.'+latest_version.patch);
		GM_setValue('lastCheck', currentTime);
		GB_hide();
		return false;
	});
	
	if (currentTime > (lastCheck + 86400) || debugUpdate == true) { // 43200 12 hours // 86400 24 hours after last check
		//alert("updateCheck");
		GMlog("updateCheck");
		//if (!isWebkit) {
			updateCheck();
		//}
	}
	//alert("LOADED!");
	
	//browser check
	//$ljq.each($ljq.browser, function(i, val) {
		//$ljq("<div>" + i + " : <span>" + val + "</span></div>").appendTo("#gmmnu");
		//alert(i+" : "+val);
	//});
//});

/*==================
// End of script
==================*/

//
// Notes
//   Created base64 images via (make sure to check base64 checkbox)
//     http://software.hixie.ch/utilities/cgi/data/data
//  The original (un-modified) update-notification script was made by Seifer
//    You can find the original at http://userscripts.org/scripts/show/12193
//    This custom version has been nearly rewritten by Daem0nX
//  Using JSON2 from http://content.worldnow.com/global/tools/video/json2.js
//    Original doesnt work in greasemonkey for Ubuntu http://www.json.org/json2.js
//  Using a modified version of Greybox Redux
//    Required: http://jquery.com/
//    Written by: John Resig
//    Based on code by: 4mir Salihefendic (http://amix.dk)
//    License: LGPL (read more in LGPL.txt)
//  Using code from http://userscripts.org/scripts/show/62634
//    for getting available formats (MIT License)
//  Using icons from "Tango Desktop Project" http://tango.freedesktop.org/Tango_Desktop_Project
//    Used in compliance with Public Domain License
//  Using icons from PC.DE http://pc.de/icons/
//    Used in compliance with CC BY http://creativecommons.org/licenses/by/3.0/
//  Using icons from "WooFunction Icon set" http://wefunction.com/contact/
//    Used in compliance with GPL
//  Using icons from "Fugue Icons" http://p.yusukekamiyamane.com/
//  Using icons from http://www.iconfinder.com/icondetails/38605/101/paypal_straight_icon
//    Used in compliance with Free for commercial use
//  Using icons from "Silk icon set"  http://www.famfamfam.com/about/
//    Used in compliance with CC BY http://creativecommons.org/licenses/by/2.5/
//

//
// Thanks
//  @UGJKA - Reported Chrome HTML5 playback issue.
//  Gandalf - Reported issue with %U using incorrect username when logged into YouTube.
//

//
// ChangeLog
// 2013.03.20 - 0.4.8
//  Added support to check if the download link is valid on click.
//    If the download link has expired a message to refresh will be displayed.
//  Fixed broken download links due to YouTube code changes.
//  Known issue in Chrome - Clicking a download link will sometimes make the video restart or freeze.
//
// 2012.12.21 - 0.4.7
//  Fixed broken download links due to YouTube code changes.
//  Refactored code that parses download links - completely rebuilds link with expected querystring variables.
//
// 2012.12.17 - 0.4.6
//  Fixed username (code %U) using incorrect value if logged into YouTube.
//
// 2012.12.06 - 0.4.5
//  Fixed username and title (code %U and %T) not working due to YouTube code changes.
//  Modified script meta data.
//    Added @grant for GM_getValue, GM_setValue, GM_deleteValue, GM_addStyle, GM_xmlhttpRequest, GM_log.
//  Upgraded jQuery from 1.8.2 to 1.8.3.
//
// 2012.09.22 - 0.4.4
//  Added format info for 17, 36.
//  Fixed video length 00 seconds showing as 0.
//  Upgraded jQuery from 1.7.2 to 1.8.2.
//
// 2012.09.13 - 0.4.3
//  Fixed broken download links.
//
// 2012.05.05 - 0.4.2
//  Fixed HTML5 video playback in Chrome.
//
// 2012.05.03 - 0.4.1
//  Added support for HTML5 videos.
//  Fixed Chrome download breaking currently playing video.
//    Download links now open in a new tab.
//  Fixed username (video title %U) not working due to YouTube code changes.
//  Fixed menu and overlay border radius not working in Firefox 13 beta.
//  Upgraded jQuery from 1.7.1 to 1.7.2.
//    Changed jQuery calls .bind() and .unbind() to .on() and .off().
//
// 2012.02.19 - 0.4.0
//  Changed name from YouTube.com - EZ Download to YouTube - EZ Download
//    Remove the old YouTube.com EZ Download from your Greasemonkey scripts!
//    Upgrade instructions overlay added.
//  Changes to Configuration overlay.
//    Added title options for user, videoid, quality, length, upload date(year,month,day).
//    Links auto regenerate after changes, no refresh required.
//  Changed Flattr donation link.
//  Fixed undeclared variable.
//  Fixed menu error message using wrong id.
//  Fixed menu blocking playlist section due to YT changes.
//  Fixed unknown formats showing as NaN, now shows format number.
//  Fixed donation section not setting the correct height if the error section was active.
//  Added format info for 46, 82, 84, 100, 102.
//  Added 3D File section.
//  Removed unused variables.
//  Upgraded jQuery from 1.6.2 to 1.7.1.
//    Changed .live() calls to .on()
//
// 2011.08.10 - 0.3.4
//  Fixed updateCheck not returning update if HTTPS Everywhere was installed.
//    Changed USO update link from HTTP to HTTPS.
//  Modified isWebkit checks to allow update notification in Chrome.
//    Chrome v13 introduced GM_xmlhttpRequest support.
//  Enabled LocalStorage for Chrome.
//    Captures GM_get/set/delete/value commands.
//
// 2011.08.05 - 0.3.3
//  Fixed broken download links.
//    Added check for new flashvar url_encoded_fmt_stream_map.
//
// 2011.08.03 - 0.3.2
//  Menu displays error as v0.3.1 didnt correctly fix downloads.
//    Working on a fix and will release as soon as possible.
//
// 2011.08.03 - 0.3.1
//  Fixed links not loading.
//    Added check for new flashvar fmt_list.
//
// 2011.08.01 - 0.3.0
//  Chrome is now partially supported.
//    Download links work, check for script updates manually.
//  Upgraded jQuery from 1.3.2 to 1.6.2
//  Fixed list being out of order (by quality).
//  Fixed overlay tabs changing url location.
//  Fixed menu blocking playlist section.
//  Fixed menu not loading on non-standard url (ex youtube.com/watch?feature=related&v=code).
//  Added file format to link description.
//  Added detection of Webkit to disable certain features in Chrome.
//  Added a replace hack to allow Configuration overlay to show above flash in Chrome.
//  Added Twitter icon to menu bar.
//  Added new Donation options.
//    Added Flattr icon, new Paypal icon.
//  Added scroll height check for Update Alert overlay.
//  Added GM_log function GMlog for console logging.
//  Added error catching code.
//  Modified menu title attr to data-title.
//    Removes annoying hover title.
//
// 2011.07.20 - 0.2.2
//  Fixed download links not loading due to new unknown formats.
//  Known issue - List is now out of order.
//  Added error catching code.
//
// 2011.03.04 - 0.2.1
//  Added Configuration icon and overlay.
//  Added Script Homepage icon to menu bar.
//  Fixed download links not loading due to YouTube code changes.
//  Fixed some undeclared variables.
//  Fixed missing functions for skipping available update.
//  Fixed debug code for Update and Settings.
//
// 2010.12.10 - 0.2.0
//  Fixed download links.
//  Less obtrusive Update Alert (icon instead of auto prompt).
//  Added menu bar with Update and Donate icons.
//
// 2010.10.07 - 0.1
//  Initial release
//