// ==UserScript==
// @name          YouTube Fullscreen Fix
// @namespace     none
// @description   Enables overlay for YouTube HTML5 player. In normal mode, click "fullscreen" or "large player" button. In embedded mode, click settings button. Also replaces youtube links so that the page is reloaded and userscripts still work after clicking another video.
// @include       http://youtube.*/watch*
// @include       http://*.youtube.com/watch*
// @include       http://www.youtube.com/embed/*
// @include       https://youtube.*/watch*
// @include       https://*.youtube.com/watch*
// @include       https://www.youtube.com/embed/*
// @include		  http://www.youtube.com/results*
// @include		  https://www.youtube.com/results*
// @include		  http://www.youtube.com/*
// @include		  https://www.youtube.com/*
// @version       $VER: Youtube Fullscreen Fix 1.19 (13.02.2014)
// @url           http://fabportnawak.free.fr/owb/scripts/YouTube_Fullscreen_Fix.js
// ==/UserScript==

var embedded = document.location.href.indexOf("/embed/") !== -1;
var allHTMLTags = document.getElementsByTagName("*");
var found = false;

function goFullscreen(e)
{
	e.preventDefault();
    e.stopPropagation();

	for (i=0; i < allHTMLTags.length; i++)
	{
		if(allHTMLTags[i].className == "video-stream html5-main-video" && allHTMLTags[i].src != "")
		{
			allHTMLTags[i].webkitEnterFullScreen();
		}
	}

	return false;
}

function bindEvent()
{
	if(!found)
	{
		for (i=0; i < allHTMLTags.length; i++)
		{
			var name = allHTMLTags[i].className;

			if (name && name.length && name.search("ytp-size-toggle-large") != -1)
			{
				found = true;
				allHTMLTags[i].addEventListener('click', goFullscreen, true);
			}
			else if (name && name.length && name.search("ytp-button-fullscreen-enter") != -1)
			{
				found = true;
				allHTMLTags[i].addEventListener('click', goFullscreen, true);
			}
			// Since FS icon is not shown in embedded mode, we use this one instead...
			else if (embedded && name && name.length && name.search("ytp-settings-button") != -1) 
			{
				found = true;
				allHTMLTags[i].addEventListener('click', goFullscreen, true);
			}
			
			if(name && name.length && name.search("spf-prefetch") != -1)
			{
				allHTMLTags[i].className = allHTMLTags[i].className.replace("spf-prefetch", "");
			}

			if(name && name.length && name.search("spf-link") != -1)
			{
				allHTMLTags[i].className = allHTMLTags[i].className.replace("spf-link", "");
			}
		}
	}
	else
	{
		window.clearInterval(timer_fix);
	}
}

// Lame workaround
var timer_fix = window.setInterval(bindEvent, 1000);
