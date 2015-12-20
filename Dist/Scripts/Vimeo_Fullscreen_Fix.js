// ==UserScript==
// @name          Vimeo Fullscreen Fix
// @namespace     none
// @description   Enables overlay for Vimeo HTML5 player.
// @include       http://vimeo.*/*
// @include       http://*.vimeo.com/*
// @include       http://www.vimeo.com/*
// @version       $VER: Vimeo Fullscreen Fix 1.0 (28.11.2013)
// @url           http://fabportnawak.free.fr/owb/scripts/Vimeo_Fullscreen_Fix.js
// ==/UserScript==

var allHTMLTags = document.getElementsByTagName("*");
var found = false;

function goFullscreen(e)
{
	e.preventDefault();
    e.stopPropagation();

	for (i=0; i < allHTMLTags.length; i++)
	{
		if(allHTMLTags[i].tagName == "VIDEO")
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
			var tag = allHTMLTags[i].tagName;

			if (name == "fullscreen" && tag == "BUTTON")
			{
				allHTMLTags[i].addEventListener('click', goFullscreen, true);
				found = true;
				break;
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
