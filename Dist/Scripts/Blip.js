// ==UserScript==
// @name           Blip HTML5 Converter
// @namespace      none
// @description    Inspired from ClickToPlugin Safari extension
// @include        http://blip.tv/*
// @version        $VER: Blip HTML5 Converter 1.0 (05.11.2011)
// @url            http://fabportnawak.free.fr/owb/scripts/Blip.js
// ==/UserScript==
/////////////////////////////////////////////////////////////////////

/* Support from CTP */

function parseWithRegExp(text, regex, processValue) { // regex needs 'g' flag
	var obj = {};
	if(!text) return obj;
	if(processValue === undefined) processValue = function(s) {return s;};
	var match;
	while(match = regex.exec(text)) {
		obj[match[1]] = processValue(match[2]);
	}
	return obj;
}
function parseFlashVariables(s) {return parseWithRegExp(s, /([^&=]*)=([^&]*)/g);}
function parseSLVariables(s) {return parseWithRegExp(s, /\s?([^,=]*)=([^,]*)/g);}

function extractExt(url) {
	var i = url.search(/[?#]/);
	if(i === -1) i = undefined;
	url = url.substring(url.lastIndexOf("/", i) + 1, i);
	i = url.lastIndexOf(".");
	if(i === -1) return "";
	return url.substring(i + 1).toLowerCase();
}

function typeInfo(type) {
	type = stripParams(type).toLowerCase();
	if(nativeMediaTypes[type]) return {"isNative": true, "isAudio": /^audio/.test(type), "format": nativeMediaTypes[type].format};
	if(addedMediaTypes[type]) return {"isNative": false, "isAudio": /^audio/.test(type), "format": addedMediaTypes[type].format};
	return null;
}

function urlInfo(url) {
	url = extractExt(url);
	if(url === "") return null;
	for(var type in nativeMediaTypes) {
		if(nativeMediaTypes[type].exts.indexOf(url) !== -1) return {"isNative": true, "isAudio": /^audio/.test(type), "format": nativeMediaTypes[type].format};
	}
	for(var type in addedMediaTypes) {
		if(addedMediaTypes[type].exts.indexOf(url) !== -1) return {"isNative": false, "isAudio": /^audio/.test(type), "format": addedMediaTypes[type].format};
	}
	return null;
}

// Shortcuts for common types
var video = document.createElement("video");
var canPlayOgg = video.canPlayType("video/ogg");
var canPlayWebM = video.canPlayType("video/webm");
var canPlayFLV = video.canPlayType("video/x-flv");
var canPlayWM = video.canPlayType("video/x-ms-wmv");
// canPlayFLAC cannot be checked

var nativeMediaTypes = {
	"video/3gpp": {"exts": ["3gp", "3gpp"], "format": "3GPP"},
	"video/3gpp2": {"exts": ["3g2", "3gp2"], "format": "3GPP2"},
	"video/avi": {"exts": ["avi", "vfw"], "format": "AVI"},
	"video/flc": {"exts": ["flc", "fli", "cel"], "format": "FLC"},
	"video/mp4": {"exts": ["mp4"], "format": "MP4"},
	"video/mpeg": {"exts": ["mpeg", "mpg", "m1s", "m1v", "m75", "m15", "mp2", "mpm", "mpv"], "format": "MPEG"},
	"video/quicktime": {"exts": ["mov", "qt", "mqv"], "format": "MOV"},
	"video/x-dv": {"exts": ["dv", "dif"], "format": "DV"},
	"video/x-m4v": {"exts": ["m4v"], "format": "M4V"},
	"video/x-mpeg": {"exts": [], "format": "MPEG"},
	"video/x-msvideo": {"exts": [], "format": "AVI"},
	"audio/3gpp": {"exts": [], "format": "3GPP"},
	"audio/3gpp2": {"exts": [], "format": "3GPP2"},
	"audio/amr": {"exts": ["amr"], "format": "AMR"},
	"audio/aac": {"exts": ["aac", "adts"], "format": "AAC"},
	"audio/ac3": {"exts": ["ac3"], "format": "AC3"},
	"audio/aiff": {"exts": ["aiff", "aif", "aifc", "cdda"], "format": "AIFF"},
	"audio/basic": {"exts": ["au", "snd", "ulw"], "format": "AU"},
	"audio/mp3": {"exts": ["mp3", "swa"], "format": "MP3"},
	"audio/mp4": {"exts": [], "format": "MP4"},
	"audio/mpeg": {"exts": ["m1a", "mpa", "m2a"], "format": "MPEG"},
	"audio/mpeg3": {"exts": [], "format": "MP3"},
	"audio/mpegurl": {"exts": ["m3u", "m3url"], "format": "M3U"},
	"audio/mpg": {"exts": [], "format": "MPEG"},
	"audio/scpls": {"exts": ["pls"], "format": "PLS"},
	"audio/wav": {"exts": ["wav", "bwf"], "format": "WAV"},
	"audio/wave": {"exts": [], "format": "WAV"},
	"audio/x-aac": {"exts": [], "format": "AAC"},
	"audio/x-ac3": {"exts": [], "format": "AC3"},
	"audio/x-aiff": {"exts": [], "format": "AIFF"},
	"audio/x-caf": {"exts": ["caf"], "format": "CAF"},
	"audio/x-gsm": {"exts": ["gsm"], "format": "GSM"},
	"audio/x-m4a": {"exts": ["m4a"], "format": "M4A"},
	"audio/x-m4b": {"exts": ["m4b"], "format": "M4B"},
	"audio/x-m4p": {"exts": ["m4p"], "format": "M4P"},
	"audio/x-m4r": {"exts": ["m4r"], "format": "M4R"},
	"audio/x-mp3": {"exts": [], "format": "MP3"},
	"audio/x-mpeg": {"exts": [], "format": "MPEG"},
	"audio/x-mpeg3": {"exts": [], "format": "MP3"},
	"audio/x-mpegurl": {"exts": [], "format": "M3U"},
	"audio/x-mpg": {"exts": [], "format": "MPEG"},
	"audio/x-scpls": {"exts": [], "format": "PLS"},
	"audio/x-wav": {"exts": [], "format": "WAV"},
	"application/mp4": {"exts": [], "format": "MP4"},
	"application/vnd.apple.mpegurl": {"exts": ["m3u8"], "format": "M3U8"}
};

var addedMediaTypes = {};
function addMediaTypes(types) {
	for(var type in types) addedMediaTypes[type] = types[type];
}

// Perian
if(canPlayFLV) addMediaTypes({
	"video/avi": {"exts": ["gvi", "vp6"], "format": "AVI"},
	"video/divx": {"exts": ["divx"], "format": "DivX"},
	"video/msvideo": {"exts": [], "format": "AVI"},
	"video/webm": {"exts": ["webm"], "format": "WebM"},
	"video/x-flv": {"exts": ["flv"], "format": "FLV"},
	"video/x-nuv": {"exts": ["nuv"], "format": "NUV"},
	"video/x-matroska": {"exts": ["mkv"], "format": "MKV"},
	"audio/webm": {"exts": [], "format": "WebM"},
	"audio/x-matroska": {"exts": ["mka"], "format": "MKA"},
	"audio/x-tta": {"exts": ["tta"], "format": "TTA"}
});
// Xiph
if(canPlayOgg) addMediaTypes({
	"video/annodex": {"exts": ["axv"], "format": "AXV"},
	"video/ogg": {"exts": ["ogv"], "format": "Ogg"},
	"video/x-annodex": {"exts": [], "format": "AXV"},
	"video/x-ogg": {"exts": [], "format": "Ogg"},
	"audio/annodex": {"exts": ["axa"], "format": "AXA"},
	"audio/ogg": {"exts": ["oga"], "format": "Ogg"},
	"audio/speex": {"exts": ["spx"], "format": "Ogg"},
	"audio/x-annodex": {"exts": [], "format": "AXA"},
	"audio/x-ogg": {"exts": [], "format": "Ogg"},
	"audio/x-speex": {"exts": [], "format": "Ogg"}
});
// Flip4Mac
if(canPlayWM) addMediaTypes({
	"video/x-ms-asf": {"exts": ["asf"], "format": "WMV"},
	"video/x-ms-asx": {"exts": ["asx"], "format": "WMV"},
	"video/x-ms-wm": {"exts": ["wm"], "format": "WMV"},
	"video/x-ms-wmv": {"exts": ["wmv"], "format": "WMV"},
	"video/x-ms-wmx": {"exts": ["wmx"], "format": "WMV"},
	"video/x-ms-wvx": {"exts": ["wvx"], "format": "WMV"},
	"audio/x-ms-wax": {"exts": ["wax"], "format": "WMA"},
	"audio/x-ms-wma": {"exts": ["wma"], "format": "WMA"}
});

function getParams(element) {
	var params = {};
	// all attributes are passed to the plugin
	// FIXME?: only no-NS attributes should be considered
	for(var i = 0; i < element.attributes.length; i++) {
		params[element.attributes[i].name.toLowerCase()] = element.attributes[i].value;
	}
	// for objects, add (and overwrite with) param children
	if(element.nodeName.toLowerCase() === "object") {
		var paramElements = element.getElementsByTagName("param");
		for(var i = 0; i < paramElements.length; i++) {
			params[paramElements[i].name.toLowerCase()] = paramElements[i].value;
		}
	}
	return params;
}

/////////////////////////////////////////////////////////////////////
// Blip Support
/////////////////////////////////////////////////////////////////////

function canKill(data) {
	return data.src.indexOf("blip.tv/") !== -1;
}

function process(data, callback) {
	if(/stratos.swf/.test(data.src)) {
		var isEmbed = false;
		var url = null;
		//var url = parseFlashVariables(data.params.flashvars).file;
		if(!url) {
			var match = /[?&#]file=([^&]*)/.exec(data.src);
			if(!match) return;
			url = match[1];
			isEmbed = true;
		}
		processXML(decodeURIComponent(url), isEmbed, callback, data);
	} else {
		var match = /blip\.tv\/play\/([^%]*)/.exec(data.src);
		if(match) processOldVideoID(match[1], callback, data);
	}
}

function processXML(url, isEmbed, callback, data) {
	var xhr = new XMLHttpRequest();
	xhr.open('GET', url, true);
	xhr.onload = function() {
		var xml = xhr.responseXML;
		var media = xml.getElementsByTagNameNS("http://search.yahoo.com/mrss/", "content");
		var sources = [];
		var url, info, height, width, audioOnly = true;

		for(var i = 0; i < media.length; i++) {
			url = media[i].getAttribute("url");
			info = urlInfo(url);
			if(!info) continue;
			if(!info.isAudio) audioOnly = false;
			height = media[i].getAttribute("height");
			width = media[i].getAttribute("width");
			info.url = url;
			info.format = media[i].getAttributeNS("http://blip.tv/dtd/blip/1.0", "role") + (info.isAudio ? "" : " (" + width + "x" + height + ")") + " " + info.format;
			info.height = parseInt(height);
			sources.push(info);
		}

		var siteInfo;
		if(isEmbed) {
			var itemId = xml.getElementsByTagNameNS("http://blip.tv/dtd/blip/1.0", "item_id")[0];
			if(itemId) siteInfo = {"name": "Blip.tv", "url": "http://www.blip.tv/file/" + itemId.textContent};
		}

		callback({
			"playlist": [{
				"title": xml.getElementsByTagName("item")[0].getElementsByTagName("title")[0].textContent,
				"poster": xml.getElementsByTagNameNS("http://search.yahoo.com/mrss/", "thumbnail")[0].getAttribute("url"),
				"sources": sources,
				"siteInfo": siteInfo
			}],
			"audioOnly": audioOnly
		}, data);
	};
	xhr.send(null);
}

function processOldVideoID(videoID, callback, data) {
	var xhr = new XMLHttpRequest();
	xhr.open('GET', "http://blip.tv/players/episode/" + videoID + "?skin=api", true);
	xhr.onload = function() {
		processXML("http://blip.tv/rss/flash/" + xhr.responseXML.getElementsByTagName("id")[0].textContent, true, callback, data);
	};
	xhr.send(null);
}

/////////////////////////////////////////////////////////////////////
// MediaPlayer
/////////////////////////////////////////////////////////////////////

function createPlayer(videoData, data) {

	var src = videoData.playlist[0].sources[0].url;
	console.warn('Blip Video URL: ' + src);

	element = document.createElement("div");
	element.className = "HTML5MediaPlayer";
	element.tabIndex = -1;

	mediaElement = document.createElement("video");
	mediaElement.className = "MediaElement";
	mediaElement.setAttribute("autoplay", "true");
	mediaElement.setAttribute("controls", "true");
	mediaElement.setAttribute("width", data.width);
	mediaElement.setAttribute("height", data.height);
	mediaElement.setAttribute("src", videoData.playlist[0].sources[0].url);

	downloadElement = document.createElement("div");
	downloadElement.innerHTML = '<a title="Download video" href="' + src + '">Download</a>';

	element.appendChild(mediaElement);
	element.appendChild(document.createElement("br"));
	element.appendChild(downloadElement);

	data.element.style.display="none";
	data.container.appendChild(element);
}

/////////////////////////////////////////////////////////////////////
// Main scope
/////////////////////////////////////////////////////////////////////

var element = document.getElementById("video_player_embed");
if(element)
{
	var src = element.src;
	element = element.parentNode;

	// Plugin data
	var data = {};

	data.src = src;
	data.type = element.type;
	data.location = location.href;
	data.isObject = element instanceof HTMLObjectElement;
	data.params = getParams(element); // parameters passed to the plugin
	data.element = element;
	data.container = element.parentNode;
	data.width = element.width;
	data.height = element.height;
	data.title = document.title;
	data.baseURL = "";

	if(canKill(data))
	{
		process(data, createPlayer);
	}
}
