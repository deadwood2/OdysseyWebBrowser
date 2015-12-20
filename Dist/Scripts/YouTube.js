// Youtube HTML5 converter
// version 0.95
// 2014-03-19
// Copyright (c) 2010, Arne Schneck, Rob Middleton(aka themiddleman), chromeuser8
// Reworked by Fabien Coeurjoly to be up-to-date and work properly with current HTML5 implementations and Youtube endless changes
// Released under the GPL license
// http://www.gnu.org/copyleft/gpl.html
//
// Changes version 0.94 -> 0.95 :
//     * Don't use for...of construct for pre-1.24 Odyssey versions.
// Changes version 0.92 -> 0.94 :
//     * Added support for videos using encrypted signatures, which should hopefully fix many videos that didn't work anymore. VEVO ones should also work.
//     * Fixed a layout issue with the video element caused by recent youtube changes.
//     * Downloading a video now shows the video title instead of a meaningless "videoplayback" filename.
//     * Code cleanup here and there. 
// Changes version 0.91 -> 0.92 :
//     * Reworked script initialization to work more reliably with all Youtube "modes" (HTML5, broken HTML5 and Flash)
// Changes version 0.90 -> 0.91 :
//     * Also suppressed more Youtube's madness : when html5 cookie is set and flash is disabled, they sometimes show a "you need flash player" message while their html5 player is already loaded ready to play the video. Seriously... 
// Changes version 0.89 -> 0.90 :
//     * Heavy refactoring to deal with all recent YouTube changes. 
// Changes version 0.88 -> 0.89 :
//     * Adapted to work again with latest changes from YouTube. 
// Changes version 0.87 -> 0.88 :
//     * Adapted to work again with latest changes from YouTube.
// Changes version 0.86 -> 0.87 :
//     * Adapted to work again with latest changes from YouTube.
// Changes version 0.85 -> 0.86 :
//     * Fixes download links after Youtube's latest changes.
// Changes version 0.84 -> 0.85 :
//     * Don't attempt to execute anything of this script in HTML5 mode. 
// Changes version 0.83 -> 0.84 :
//     * YouTube suppressed the url_fmt_map this script relied on previously... 
//       Now using another method, let's hope it won't change too soon!      
//     * Removed some dead code and cleanup in a few places 
// Changes version 0.82 -> 0.83 :
//     * Added very basic support for YouTube channels (only first title is played).
// Changes version 0.81 -> 0.82 :
//	   * The player container height set by YouTube is too small to show entirely
//       the HTML5 bultin-player controls. Adjusted its height accordingly.
// Changes version 0.80 -> 0.81 :
//     * "get_video" mode isn't supported by YouTube anymore, so "raw" mode is now
//       the default and only available mode.
//
// ==UserScript==
// @name          Youtube HTML5 Converter
// @namespace     none
// @description   Adds links below the video and replaces Flash with the builtin HTML5 mediaplayer.
// @include       http://youtube.com/watch*
// @include		  http://*.youtube.com/watch*
// @include       http://youtube.*/watch*
// @include		  http://youtube-nocookie.com/watch*
// @include		  http://*.youtube-nocookie.com/watch*
// @include		  http://youtube-nocookie.*/watch*
// @include		  http://*.youtube.com/user/*
// @include		  http://www.youtube.com/results*
// @include		  http://www.youtube.com/*
// @include       https://youtube.com/watch*
// @include		  https://*.youtube.com/watch*
// @include       https://youtube.*/watch*
// @include		  https://youtube-nocookie.com/watch*
// @include		  https://*.youtube-nocookie.com/watch*
// @include		  https://youtube-nocookie.*/watch*
// @include		  https://*.youtube.com/user/*
// @include		  https://www.youtube.com/results*
// @include		  https://www.youtube.com/*
// @version		  $VER: Youtube HTML5 converter 0.95 (19.03.2014)
// @url			  http://fabportnawak.free.fr/owb/scripts/Youtube.js
// ==/UserScript==

var startupDelay = 500; // delay in ms before the script kicks in (lame workaround for lame YouTube behaviour).

/*------------------------------------------------------------------------------------------------------------------*/

function GM_addScript(contents, id, isurl) {
	var head, script;
	head = document.getElementsByTagName('head')[0];
	if (!head) { return; }
	script = document.getElementById(id);
	if(script != undefined) {
		head.removeChild(script);
	}
	script = document.createElement('script');
	script.type = 'text/javascript';
	script.id = id;
	if(isurl) {
		script.src = contents
	} else {
		script.innerHTML = contents;
	}
	head.appendChild(script);
}

//http://diveintogreasemonkey.org/patterns/add-css.html
function GM_addGlobalStyle(css) {
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
	window.console.log(text);
}

/*------------------------------------------------------------------------------------------------------------------*/

/*
 * pMan preferences manager for greasemonkey scripts
 * http://userscripts.org/scripts/show/71904
 */
var pMan=function(a){var d=this;d.parentElm=null;d.PMan=function(){if(a.elmId)d.parentElm=document.getElementById(a.elmId);else{var c=document.createElement("div");c.style.width="300px";c.style.position="fixed";c.style.left="50%";c.style.marginLeft="-150px";c.style.top="150px";document.getElementsByTagName("body")[0].appendChild(c);d.parentElm=c}};d._save=function(){for(var c=0;c<a.prefs.length;c++){var b=document.getElementById("pManOption"+c);GM_setValue(a.prefs[c].name,b.value)}return false};d._hide= function(){d.parentElm.style.display="none";return false};d._savehide=function(){d._save();d._hide();return false};d.show=function(){for(var c="<div style='"+(a.bordercolor?"border:1px solid "+a.bordercolor+";":"")+(a.color?"color:"+a.color+";":"")+(a.bgcolor?"background-color:"+a.bgcolor+";":"")+"padding:3px;'><div style='font-weight:bold;text-align:center;padding:3px;'>"+(a.title||"")+"</div>",b=0;b<a.prefs.length;b++){c+="<div title='"+(a.prefs[b].description||"")+"'>"+a.prefs[b].name+" <select id='pManOption"+ b+"'>";for(var e=0;e<a.prefs[b].opts.length;e++)c+="<option value='"+(a.prefs[b].vals?a.prefs[b].vals[e]:a.prefs[b].opts[e])+"'>"+a.prefs[b].opts[e]+"</option>";c+="</select></div>"}c+="<div style='text-align:right'><a href='#' id='pManButtonCancel'>Cancel</a> <a href='#' id='pManButtonSave'>Save</a></div></div>";d.parentElm.innerHTML=c;document.getElementById("pManButtonCancel").addEventListener("click",d._hide,true);document.getElementById("pManButtonSave").addEventListener("click",d._savehide, true);for(b=0;b<a.prefs.length;b++)document.getElementById("pManOption"+b).value=d.getVal(a.prefs[b].name);d.parentElm.style.display=""};d.getVal=function(c){for(var b=0;b<a.prefs.length;b++)if(a.prefs[b].name==c)return a.prefs[b].vals?GM_getValue(a.prefs[b].name,a.prefs[b].vals[a.prefs[b].defaultVal]):GM_getValue(a.prefs[b].name,a.prefs[b].opts[a.prefs[b].defaultVal]);return"pref default doesnt exist"};d.PMan()};

/*
 * Players
 * The default is the generic player that should work with whatever 
 *  plugin is installed.
 * All should have the following:
 *
 * string name
 * 
 * string desctiption
 * 
 * function init
 * arguments: (none)
 * returns "this" player object
 * 
 * function writePlayer
 * arguments: (string) id of the parent element to fill with player
 *            (string) video url
 *            (string) autoplay (true or false)
 *            (string) width (number and 'px')
 *            (string) height (number and 'px')
 * returns: none
 * 
 * function seek
 * arguments: (int) time to seek to in seconds
 * returns: none
 * (If this doesnt work just use an empty function anyway.)
 */

var playersAvailable = [{
	name:"HTML5 Player",
	description:"HTML5 Builtin player",
	init:function() {
		return this;
	},
	writePlayer:function(parentDivId, url, autoplay, width, height) {
		var parentDiv = document.getElementById(parentDivId);
		var s_autoplay = autoplay ? "autoplay" : "";
		parentDiv.innerHTML = '<video id="movie_player" src="' + url + '" width="' + parseInt(width) + '" height="' + parseInt(height) + '"' + s_autoplay + ' controls></video>';
	},
	seek:function(seconds) {
		// should get added someday
	}
}];

// Unused table, here for reference
var formats = {
	5:  { itag: 5 , quality: 5, description: "Low Quality, 240p"     			, format: "FLV" 		, fres: "240p", 	mres: { width:  400, height:  240 }, acodec: "MP3"   , vcodec: "SVQ"   },
	17: { itag: 17, quality: 4, description: "Low Quality, 144p"				, format: "3GP" 		, fres: "144p", 	mres: { width:  0, height: 0  }, acodec: "AAC"   , vcodec: "" },
	18: { itag: 18, quality: 15, description: "Low Definition, 360p"			, format: "MP4" 		, fres: "360p", 	mres: { width:  480, height:  360 }, acodec: "AAC"   , vcodec: "H.264" },
	22: { itag: 22, quality: 35, description: "High Definition, 720p" 			, format: "MP4" 		, fres: "720p", 	mres: { width: 1280, height:  720 }, acodec: "AAC"   , vcodec: "H.264" },
	34: { itag: 34, quality: 10, description: "Low Definition, 360p"  			, format: "FLV" 		, fres: "360p", 	mres: { width:  640, height:  360 }, acodec: "AAC"   , vcodec: "H.264" },
	35: { itag: 35, quality: 25, description: "Standard Definition, 480p"  		, format: "FLV" 		, fres: "480p", 	mres: { width:  854, height:  480 }, acodec: "AAC"   , vcodec: "H.264" },
	36: { itag: 36, quality: 6, description: "Low Quality, 240p"  				, format: "3GP" 		, fres: "240p", 	mres: { width:  0, height:  0 }, acodec: "AAC"   , vcodec: "" },
	37: { itag: 37, quality: 45, description: "Full High Definition, 1080p"		, format: "MP4" 		, fres: "1080p", 	mres: { width: 1920, height: 1080 }, acodec: "AAC"   , vcodec: "H.264" },
	38: { itag: 38, quality: 55, description: "Original Definition" 			, format: "MP4" 		, fres: "Orig", 	mres: { width: 4096, height: 3072 }, acodec: "AAC"   , vcodec: "H.264" },
	43: { itag: 43, quality: 20, description: "Low Definition, 360p"  			, format: "WebM"		, fres: "360p", 	mres: { width:  640, height:  360 }, acodec: "Vorbis", vcodec: "VP8"   },
	44: { itag: 44, quality: 30, description: "Standard Definition, 480p"		, format: "WebM"		, fres: "480p", 	mres: { width:  854, height:  480 }, acodec: "Vorbis", vcodec: "VP8"   },
	45: { itag: 45, quality: 40, description: "High Definition, 720p" 			, format: "WebM"		, fres: "720p", 	mres: { width: 1280, height:  720 }, acodec: "Vorbis", vcodec: "VP8"   },
	46: { itag: 46, quality: 50, description: "Full High Definition, 1080p"		, format: "WebM"		, fres: "1080p", 	mres: { width: 1280, height:  720 }, acodec: "Vorbis", vcodec: "VP8"   },
	82: { itag: 82, quality: 16, description: "Low Definition 3D, 360p"			, format: "MP4"			, fres: "360p",		mres: { width: 640,  height:  360 }, acodec: "AAC"	 , vcodec: "H.264" },
	84: { itag: 84, quality: 41, description: "High Definition 3D, 720p"		, format: "MP4"			, fres: "720p",		mres: { width: 1280, height:  720 }, acodec: "AAC"	 , vcodec: "H.264" },
	100: { itag: 100, quality: 17, description: "Low Definition 3D, 360p"		, format: "WebM"		, fres: "360p", 	mres: { width: 640,  height:  360 }, acodec: "Vorbis", vcodec: "VP8"   },
	102: { itag: 102, quality: 42, description: "High Definition 3D, 720p"		, format: "WebM"		, fres: "720p", 	mres: { width: 1280, height:  720 }, acodec: "Vorbis", vcodec: "VP8"   },
    133: { itag: 133, quality: 133, description: "Low Definition, 240p"	    	, format: "DASH MP4"	, fres: "240p", 	mres: { width: 400,  height:  240 }, acodec: "",       vcodec: "H.264" },
    134: { itag: 134, quality: 134, description: "Low Definition, 360p"			, format: "DASH MP4"	, fres: "360p", 	mres: { width: 480,  height:  360 }, acodec: "",       vcodec: "H.264" },
    135: { itag: 135, quality: 135, description: "Standard Definition, 480p"	, format: "DASH MP4"	, fres: "480p", 	mres: { width: 854,  height:  480 }, acodec: "",       vcodec: "H.264" },
    136: { itag: 136, quality: 136, description: "High Definition, 720p"		, format: "DASH MP4"	, fres: "720p", 	mres: { width: 1280, height:  720 }, acodec: "",       vcodec: "H.264" },
    137: { itag: 137, quality: 137, description: "Full Definition, 1080p"		, format: "DASH MP4"	, fres: "1080p", 	mres: { width: 1920, height: 1080 }, acodec: "",       vcodec: "H.264" },

    139: { itag: 139, quality: 139, description: "Low Bitrate, 48k"				, format: "DASH MP4"	, fres: "48k", 		mres: { width: 0, height:0 }, 		 acodec: "AAC",    vcodec: "" },
    140: { itag: 140, quality: 140, description: "Standard Bitrate, 128k"		, format: "DASH MP4"	, fres: "128k", 	mres: { width: 0, height:0 }, 		 acodec: "AAC",    vcodec: "" },
    141: { itag: 141, quality: 141, description: "High Bitrate, 256k"			, format: "DASH MP4"	, fres: "256k", 	mres: { width: 0, height:0 }, 		 acodec: "AAC",    vcodec: "" },
    160: { itag: 160, quality: 160, description: "High Bitrate, 192k"			, format: "DASH MP4"	, fres: "192k", 	mres: { width: 0, height:0 }, 		 acodec: "H.264",  vcodec: "" },
    171: { itag: 171, quality: 171, description: "Standard Bitrate, 128k"		, format: "DASH Vorbis"	, fres: "128k", 	mres: { width: 0, height:0 }, 		 acodec: "OGG",    vcodec: "" },
    172: { itag: 12,  quality: 172, description: "High Bitrate, 192k"			, format: "DASH Vorbis"	, fres: "192k", 	mres: { width: 0, height:0 }, 		 acodec: "OGG",    vcodec: "" },
	}
	
var orderedformats = [];

var success = false;

//////////////////////////////////////////////////////////

function qr(sr) {
  var qa = [];
  var params = sr.split('&');
  for (i = 0; i < params.length; i++) {
	prs = params[i];
    var pra = prs.split('=');
    qa[pra[0]] = pra[1];
  }
  return qa;
}

function dc(sg) {
  var xhr = new XMLHttpRequest();
  /* cors-anywhere.herokuapp.com */
  px = 'allow-any-origin.appspot.com/https:';
  xhr.open('get', 'https://' + px + ytplayer.config.assets.js, false);
  xhr.send();
  var rpt = xhr.responseText;
  var fcnm = rpt.match(/signature=([^(]+)/)[1];
  var fs = new RegExp('function ' + fcnm + '[^}]+}[^}]+}');
  eval(rpt.match(fs)[0]);
  return eval(fcnm + '("' + sg + '")');
}

////////////////////////////////////////////////////////////////////////////////////

function parseLink(url, title, num){
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
	url = decodeURIComponent(url);
	url = url.replace("url=", "").replace("sig=", "signature=");
	if (url.indexOf("http") != -1) {
		zztmp = url.split("&");
		url = "";
		for (var j = 0; j < zztmp.length; j ++)	{
			if (/^(type)/.test(zztmp[j])) {
			} else {
				if (/^(http)/.test(zztmp[j])) {
					url = zztmp[j] + url;
				} else {
					url = url + "&" + zztmp[j];
				}
			}
		}

		pattern = /itag=([0-9]+)/ig;
		matches = url.match(pattern);
		itag = matches[0].replace(pattern, "$1");
		orderedformats[num] = {};
		orderedformats[num]["itag"] = itag;

		url = encodeURI(url);
		url = url.replace(/%5Cu0026/ig, "&");
		url = url.replace(/%252C/ig, ",");
		url = url.replace(/\&itag=[0-9]+/ig, "");
		url = url + "&itag=" + itag + "&begin=0";
		
		GMlog(url);
		
		orderedformats[num]["url"] = url;
		orderedformats[num]["itag"] = itag;
		orderedformats[num]["title"] = title;
	}
}

function getFormatName(quality) {
	var ret = "(" + quality.toString() + ") unknown";
	switch(quality)
	{
		case 5:
			ret = "(" + quality.toString() + ") FLV H.263 400x240";
			break;
		case 34:
			ret = "(" + quality.toString() + ") FLV H.264 640x360";
			break;	
		case 35:
			ret = "(" + quality.toString() + ") FLV H.264 854x480";
			break;	
		case 18:
			ret = "(" + quality.toString() + ") MP4 H.264 640x360";
			break;	
		case 22:
			ret = "(" + quality.toString() + ") MP4 H.264 1280x720";
			break;	
		case 37:
			ret = "(" + quality.toString() + ") MP4 H.264 1920x1080";
			break;	
		case 38:
			ret = "(" + quality.toString() + ") MP4 H.264 4096x3072";
			break;	
		case 43:
			ret = "(" + quality.toString() + ") WEBM VP8 640x360";
			break;	
		case 44:
			ret = "(" + quality.toString() + ") WEBM VP8 854x480";
			break;	
		case 45:
			ret = "(" + quality.toString() + ") WEBM VP8 1280x720";
			break;
		case 46:
			ret = "(" + quality.toString() + ") WEBM VP8 1920x1080";
			break;
		case 17:
			ret = "(" + quality.toString() + ") 3GP MP4 176x144";
			break;																										
	}
	return ret;
}

function getFormatExtension(quality) {
	var ret = ".unknown";
	switch(parseInt(quality))
	{
		case 5:
			ret = ".flv";
			break;
		case 34:
			ret = ".flv";
			break;	
		case 35:
			ret = ".flv";
			break;	
		case 18:
			ret = ".mp4";
			break;	
		case 22:
			ret = ".mp4";
			break;	
		case 37:
			ret = ".mp4";
			break;	
		case 38:
			ret = ".mp4";
			break;	
		case 43:
			ret = ".webm";
			break;	
		case 44:
			ret = ".webm";
			break;	
		case 45:
			ret = ".webm";
			break;
		case 46:
			ret = ".webm";
			break;
		case 17:
			ret = ".3gp";
			break;																										
	}
	return ret;
}


function getHTML5Object()
{
	var ret = null;
	
	var elements = document.getElementsByTagName("video");

	if(elements.length > 0)
	{
		var element = elements[0]; 

		if(element.className == "video-stream html5-main-video")
		{
			ret = element;
		}
	}
	
	return ret;
}

function isUselessHTML5Object(obj)
{
	return obj == null || obj.src == "";
}

function getFlashObject()
{
	var ret = null;
	
	var element = document.getElementById("movie_player");
	
	if(element && element.attributes['flashvars'])
	{
		ret = element;
	}
	
	return ret;
}

function runScript() {

if(success)
{ 
	return;
}

if(!HTML5obj && !Flashobj)
	return;

var views = document.getElementById('watch-ratings-views');

if(!views)
	views = document.getElementById('watch7-views-info');
	
if(!views)
	views = document.getElementById('playnav-video-details');

if(!views)
	return;

var prefloc = document.createElement("div");

prefloc.style.width = "300px";
prefloc.style.margin = "auto";
prefloc.id = "ywofpreferences";
	
views.parentNode.insertBefore(prefloc, views);

var prefMan = new pMan({
	title:"Youtube HTML5 Converter Preferences",
	color:"black",
	bgcolor:"white",
	bordercolor:"black",
	prefs:[
		{
			name:"Default Quality",
			description:"If the default quality is not available the next best quality video will be played",
			opts:[	getFormatName(5),getFormatName(34),getFormatName(35),
					getFormatName(18),getFormatName(22),getFormatName(37),getFormatName(38),
					getFormatName(43),getFormatName(44),getFormatName(45),getFormatName(46),
					getFormatName(17),
					"Flash"
				 ],
			vals:["5","34","35","18","22","37","38","43","44","45","17","Flash"],
			defaultVal:3
		},{
			name:"Autoplay",
			description:"",
			opts:["On","Off"],
			vals:["true","false"],
			defaultVal:0
		},{
			name:"Player Size",
			description:"The size of the player",
			opts:["small","big"],
			defaultVal:0
		}
	],
	elmId:"ywofpreferences"
});

// Add css for links.
// This is probably the best way to make text look like links, using the
// normal ways wont work in GM because there is nowhere to put 
// "return false;" in the link.
GM_addGlobalStyle(".link{color:#0033CC;}" + 
		".link:hover{cursor: pointer; text-decoration:underline;}");

var defaultQuality = prefMan.getVal("Default Quality");

// Retrieve direct links

GMlog("Available YouTube direct links:");

var urlsAvailable = new Array();
var formatsAvailable = new Array();
var titlesAvailable = new Array();

// Retrieving direct links
var args = ytplayer.config.args;
i = 0;

stream_maps = new Array(args.url_encoded_fmt_stream_map, args.adaptive_fmts);
for (j = 0; j < stream_maps.length; j++) {
  var ft = stream_maps[j];
  if(ft)
  {
	  var ft_params = ft.split(',');
	  for (k = 0; k < ft_params.length; k++) {
		z = ft_params[k];
		var qs = qr(z);
		var href = unescape(qs.url);
		try 
		{
			if(qs.itag <= 130)
			{
				if (qs.sig)
				  href += '&signature=' + qs.sig;
				if (qs.s)
				  href += '&signature=' + dc(qs.s);
				  
				parseLink(href, args.title, i++);
			}
		}
		catch(e)
		{
		}		
	  }
  }
}

var default_url = "";
for (var i = 0; i < orderedformats.length; i ++) {	
	urlsAvailable[i] = orderedformats[i]["url"];
	formatsAvailable[i] = orderedformats[i]["itag"];
	titlesAvailable[i] = orderedformats[i]["title"] + getFormatExtension(orderedformats[i]["itag"]);
		
	if(orderedformats[i]["itag"] == "18")
	{
		default_url = orderedformats[i]["url"];
	}
}

GMlog("HTML5 "+HTML5obj+" Flash "+Flashobj);

if(!isUselessHTML5Object(HTML5obj))
{
	success = true;
}
else if(Flashobj || isUselessHTML5Object(HTML5obj))
{
	var playerDiv = null;
	
	if(!playerDiv)
		playerDiv = document.getElementById("player-api");		
		
	// Last attempt, try replacing useless Youtube's HTML5 player.
	if(!playerDiv && isUselessHTML5Object(HTML5obj))
		playerDiv = document.getElementById("movie_player");

	var playerDivLoad = playerDiv.innerHTML; // For restoring flash.
	var activePlayer = playersAvailable[0].init();

	if(defaultQuality != "flash") {
		// If they don't want flash clear it asap so it doesn't start autoplaying.
		playerDiv.innerHTML = "";
	}

	function writePlayer(quality) {

		var width  = "640px";
		var height = "388px";
		
		var qualityId = -1;
		for(i = 0; i<formatsAvailable.length; i++)
		{ 
			if(formatsAvailable[i] == quality)
			{
				qualityId = i;
				break;
			}
		}

		activePlayer.writePlayer(playerDiv.id,
				urlsAvailable[qualityId],
				prefMan.getVal("Autoplay"),
				width,
				height);
	}

	function restoreFlash() {
		playerDiv.innerHTML = playerDivLoad;
	}

	var haveFlash;
	var noplayerDiv = document.getElementById("watch-noplayer-div");
	if(noplayerDiv == null) {
		haveFlash = true;
	} 
	else {
		haveFlash = false;
	}

	var linkbar = document.createElement("div");
	var linkViewFlash = "";
	var linkViewPreferences = "";
	var downloadLinks = "";
	var playLinks = "";

	for(var i = 0; i < formatsAvailable.length; i++) {
		if(typeof(urlsAvailable[i]) != "undefined") {
			downloadLinks += '| <a href="' + urlsAvailable[i] + '" download="' + titlesAvailable[i] + '">' + 
					formatsAvailable[i].toString() + '</a> ';
			
			playLinks += '| <a class="link" id="play' + formatsAvailable[i] + '">' + 
					formatsAvailable[i].toString() + '</a> ';
		}
	}

	if(haveFlash) {
		linkViewFlash = '<br/><a class="link" id="restoreFlash">View Flash</a>';
	}

	linkViewPreferences = '<a class="link" id="preferencesLink">Preferences</a>';
	linkbar.innerHTML = '<div id="dlbar" style="padding-top: 8px;">'
		+ 'Download '
		+ downloadLinks
		+ '<br/>View as HTML5 '
		+ playLinks
		+ linkViewFlash
		+ '<div style="float:right;">' + linkViewPreferences + '</div>'
		+ '</div>';

	views.parentNode.insertBefore(linkbar,views);

	for(var i = 0; i < formatsAvailable.length; i++) {
		if(typeof(urlsAvailable[i]) != "undefined") {
			var playLink = document.getElementById('play' + formatsAvailable[i]);
			var writePlayerFunction = function(qual) {
				return function (event) {
					writePlayer(qual);
				};
			};
			playLink.addEventListener("click", writePlayerFunction(formatsAvailable[i]), true);
		}
	}

	if(haveFlash) {
		var restoreFlashLink = document.getElementById('restoreFlash');
		restoreFlashLink.addEventListener("click", restoreFlash, true);
	}

	var preferencesLink = document.getElementById('preferencesLink');
	preferencesLink.addEventListener("click", prefMan.show, true);


	// Finally, write the player, if the desired format is not available we 
	// keep going down in quality until we find one.

	if(defaultQuality != "flash") {

		var defaultQualityId = -1;
		
		for(i = 0; i < formatsAvailable.length; i++)
		{ 
			if(formatsAvailable[i] == parseInt(defaultQuality))
			{
				defaultQualityId = i;
				break;
			}
		}

		if(defaultQualityId < 0)
			format = 18; // Try to force it anyway
		else
			format = formatsAvailable[defaultQualityId]; 
			
		writePlayer(format);	
	}	

	// Make sure we remove explicitely the HTMLMediaElement, since it can be leaked in some cases
	function pageUnloaded()
	{
		playerDiv.removeChild(playerDiv.firstChild);
	}

	window.addEventListener("unload", pageUnloaded, false);
	
	success=true;	
}	

}

var timer;
var HTML5obj = getHTML5Object();
var Flashobj = getFlashObject();

// Wait until HTML5 player is there to run the script.
function checkHTML5Object()
{
	HTML5obj = getHTML5Object();

	if(HTML5obj)
	{
		FlashObj = null;
	
		runScript();
		window.clearInterval(timer);
	}
}

timer = window.setInterval(checkHTML5Object, 500);

function handleBeforeLoadEvent(event) {
    var element = event.target;

	if(element instanceof HTMLEmbedElement) 
	{
		runScript();
	}
}

document.addEventListener("beforeload", handleBeforeLoadEvent, true);

function startup()
{
	if(Flashobj)
	{
		// Erase it, this will cause the Flash element to be loaded again, caught in beforeloadevent.
		var obj = document.getElementById("player-api");
		if(obj)
			obj.innerHTML = "";
	}
}

startup();







