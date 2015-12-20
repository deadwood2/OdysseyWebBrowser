// ==UserScript==
// @name           Facebook HTML5 Converter
// @namespace      none
// @description    Inspired from ClickToPlugin Safari extension
// @include        http://*.facebook.com/*
// @version        $VER: Facebook HTML5 Converter 1.0 (16.08.2011)
// @url            http://fabportnawak.free.fr/owb/scripts/Facebook.js
// ==/UserScript==
/////////////////////////////////////////////////////////////////////

function getAttributes(element, url) {
    // Gathers essential attributes of the element that are needed to decide blocking.
    // Done by a single function so that we only loop once through the <param> children.
    // NOTE: the source used by Safari to choose a plugin is always 'url'; the value info.src
    // returned by this function is the source that is relevant for whitelisting
    var info = new Object();
    info.type = element.type;
    var tmpAnchor = document.createElement("a");
    switch (element.nodeName.toLowerCase()) {
        case "embed":
            if(element.hasAttribute("qtsrc")) {
                tmpAnchor.href = element.getAttribute("qtsrc");
                info.src = tmpAnchor.href;
            }
            if(element.hasAttribute("autohref")) {
                info.autohref = /^true$/i.test(element.getAttribute("autohref"));
            }
            if(element.hasAttribute("href")) {
                tmpAnchor.href = element.getAttribute("href");
                info.href = tmpAnchor.href;
            }
            if(element.hasAttribute("target")) {
                info.target = element.getAttribute("target");
            }
            break;
        case "object":
            var paramElements = element.getElementsByTagName("param");
            for (var i = 0; i < paramElements.length; i++) {
                if(!paramElements[i].hasAttribute("value")) continue;
                /* NOTE 1
                The 'name' attribute of a <param> element is mandatory.
                However, Safari will load an <object> element even if it has <param> children with no 'name',
                so we have to account for this possibilty, otherwise CTP could easily be circumvented!
                For these reasons we use try/catch statements.
                */
                try{
                    var paramName = paramElements[i].getAttribute("name").toLowerCase();
                    switch(paramName) {
                        case "type": // to be fixed in WebKit?
                            if(!info.type) info.type = paramElements[i].getAttribute("value");
                            break;
                        case "source": // Silverlight true source
                            tmpAnchor.href = paramElements[i].getAttribute("value");
                            info.src = tmpAnchor.href;
                            break;
                        case "qtsrc": // QuickTime true source
                            tmpAnchor.href = paramElements[i].getAttribute("value");
                            info.src = tmpAnchor.href;
                            break;
                        case "autohref": // QuickTime redirection
                            info.autohref = /^true$/i.test(paramElements[i].getAttribute("value"));
                            break;
                        case "href": // QuickTime redirection
                            tmpAnchor.href = paramElements[i].getAttribute("value");
                            info.href = tmpAnchor.href;
                            break;
                        case "target": // QuickTime redirection
                            info.target = paramElements[i].getAttribute("value");
                            break;
                    }
                } catch(err) {}
            }
            break;
    }
    if(!info.src) {
        if(!url) info.src = "";
        else {
            tmpAnchor.href = url;
            info.src = tmpAnchor.href;
        }
    }
    return info;
}

function getParams(element, plugin) {
    switch(plugin) {
        case "Flash": // need flashvars
            switch (element.nodeName.toLowerCase()) {
                case "embed":
                    return (element.hasAttribute("flashvars") ? element.getAttribute("flashvars") : ""); // fixing Safari's buggy JS
                    break
                case "object":
                    var paramElements = element.getElementsByTagName("param");
                    for (var i = paramElements.length - 1; i >= 0; i--) {
                        try{ // see NOTE 1
                            if(paramElements[i].getAttribute("name").toLowerCase() === "flashvars") {
                                return paramElements[i].getAttribute("value");
                            }
                        } catch(err) {}
                    }
                    return "";
                    break;
            }
            break;
        case "Silverlight": // need initparams
            if(element.nodeName.toLowerCase() !== "object") return "";
            var paramElements = element.getElementsByTagName("param");
            for (var i = 0; i < paramElements.length; i++) {
                try { // see NOTE 1
                    if(paramElements[i].getAttribute("name").toLowerCase() === "initparams") {
                        return paramElements[i].getAttribute("value").replace(/\s+/g,"");
                    }
                } catch(err) {}
            }
            return "";
            break;
        case "DivX": // need previewimage
            switch(element.nodeName.toLowerCase()) {
                case "embed":
                    return element.getAttribute("previewimage");
                    break
                case "object":
                    var paramElements = element.getElementsByTagName("param");
                    for (var i = 0; i < paramElements.length; i++) {
                        try{ // see NOTE 1
                            if(paramElements[i].getAttribute("name").toLowerCase() === "previewimage") {
                                return paramElements[i].getAttribute("value");
                            }
                        } catch(err) {}
                    }
                    return "";
                    break;
            }
        default: return "";
    }
}

function parseWithRegExp(string, regex, process) { // regex needs 'g' flag
    if(process === undefined) process = function(s) {return s;};
    var match;
    var obj = new Object();
    while((match = regex.exec(string)) !== null) {
        obj[match[1]] = process(match[2]);
    }
    return obj;
}
function parseFlashVariables(s) {return parseWithRegExp(s, /([^&=]*)=([^&]*)/g);}

/////////////////////////////////////////////////////////////////////
// Facebook Support
/////////////////////////////////////////////////////////////////////

function canKill(data) {
    if(data.plugin !== "Flash") return false;
    return /^https?:\/\/(?:s-static\.ak\.facebook\.com|b\.static\.ak\.fbcdn\.net|static\.ak\.fbcdn\.net)\/rsrc\.php\/v[1-9]\/[a-zA-Z0-9]{2}\/r\/[a-zA-Z0-9_-]*\.swf/.test(data.src) || data.src.indexOf("www.facebook.com/v/") !== -1;
}

function process(data, callback) {
	if(data.params) {
		processFlashVars(parseFlashVariables(data.params), callback, data);
	}

	/*
    if(data.params) {
        var flashvars = parseFlashVariables(data.params);
		if(flashvars.video_href && flashvars.video_id) processVideoID(flashvars.video_id, callback, data);
		else processFlashVars(flashvars, callback, data);
        return;
    }
    // Embedded video
    var match = data.src.match(/\.com\/v\/([^&?]+)/);
	if(match) processVideoID(match[1], callback, data);
	*/
}

function processFlashVars(flashvars, callback, data) {
    var sources = new Array();
    var isHD = flashvars.video_has_high_def === "1";
    if(flashvars.highqual_src) {
        sources.push({"url": decodeURIComponent(flashvars.highqual_src), "format": isHD ? "720p MP4" : "HQ MP4", "height": isHD ? 720 : 600, "isNative": true, "mediaType": "video"});
        if(flashvars.lowqual_src) sources.push({"url": decodeURIComponent(flashvars.lowqual_src), "format": "240p MP4", "height": 240, "isNative": true, "mediaType": "video"});
    } else if(flashvars.video_src) {
        sources.push({"url": decodeURIComponent(flashvars.video_src), "format": "240p MP4", "height": 240, "isNative": true, "mediaType": "video"});
    } else return;

    var posterURL, title;
    if(flashvars.thumb_url) posterURL = decodeURIComponent(flashvars.thumb_url);
    if(flashvars.video_title) title = decodeURIComponent(flashvars.video_title).replace(/\+/g, " ");
    var videoData = {
        "playlist": [{"title": title, "poster": posterURL, "sources": sources}]
    };
	callback(videoData, data);
}

/////////////////////////////////////////////////////////////////////
// MediaPlayer
/////////////////////////////////////////////////////////////////////

function createPlayer(videoData, data) {

	console.warn('Facebook Video URL: ' + videoData.playlist[0].sources[0].url);

	element = document.createElement("div");
	element.className = "HTML5MediaPlayer";
	element.tabIndex = -1;

	mediaElement = document.createElement("video");
	mediaElement.className = "MediaElement";
	mediaElement.setAttribute("autoplay", "true");
	mediaElement.setAttribute("controls", "true");
	mediaElement.setAttribute("src", videoData.playlist[0].sources[0].url);

	element.appendChild(mediaElement);

	data.element.parentNode.style.display="none";
	data.container.appendChild(element);
}

/////////////////////////////////////////////////////////////////////
// Main scope
/////////////////////////////////////////////////////////////////////

document.addEventListener("beforeload", handleBeforeLoadEvent, true);

function handleBeforeLoadEvent(event) {
    var element = event.target;

	if(!(element instanceof HTMLObjectElement || element instanceof HTMLEmbedElement)) return;

	if(element.getAttribute("classid")) return; // new behavior in WebKit

    var data = getAttributes(element, event.url);

    data.url = event.url;
    data.width = element.offsetWidth;
    data.height = element.offsetHeight;
    data.location = window.location.href;
    data.className = element.className;
	data.plugin = "Flash";
	data.params = getParams(element, data.plugin);
	data.element = element;
	data.container = element.parentNode.parentNode;

	if(canKill(data))
	{
	    event.preventDefault(); // prevents 'element' from loading
	    event.stopImmediatePropagation(); // compatibility with other extensions
		process(data, createPlayer);
	}
}

