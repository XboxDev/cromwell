<!doctype html public "-//W3C//DTD HTML 4.0 Transitional//EN"
"http://www.w3.org/TR/REC-html40/loose.dtd">
<html><head>
<!-- ViewCVS -- http://viewcvs.sourceforge.net/
by Greg Stein -- mailto:gstein@lyra.org
-->
<title>CVS log for grub/grub/stage2/xfs.h</title>
</head>
<body text="#000000" bgcolor="#ffffff">
<table width="100%" border=0 cellspacing=0 cellpadding=0>
<tr>
<td rowspan=2><h1>CVS log for grub/grub/stage2/xfs.h</h1></td>
<td align=right><a href="/"><img src="/images/transparent.theme/floating.png" alt="Savannah" border="0" width="150" height="130"></a></td>
</tr>
<tr>
<td align=right><h3><b><a target="_blank"
href="/cgi-bin/viewcvs/*docroot*/help_log.html">Help</a></b></h3></td>
</tr>
</table>
<a href="/cgi-bin/viewcvs/grub/grub/stage2/#xfs.h"><img src="/viewcvs/icons/back.png" alt="(back)" border=0
width=16 height=16></a>
<b>Up to <a href="/cgi-bin/viewcvs/#dirlist">[Sources]</a> / <a href="/cgi-bin/viewcvs/grub/#dirlist">grub</a> / <a href="/cgi-bin/viewcvs/grub/grub/#dirlist">grub</a> / <a href="/cgi-bin/viewcvs/grub/grub/stage2/#dirlist">stage2</a></b><p>
<a href="#diff">Request diff between arbitrary revisions</a>

<hr noshade>

Default branch: MAIN
<br>
Bookmark a link to:

<a href="xfs.h?rev=HEAD&content-type=text/vnd.viewcvs-markup"><b>HEAD</b></a>
/
(<a href="/cgi-bin/viewcvs/*checkout*/grub/grub/stage2/xfs.h?rev=HEAD&content-type=text/plain" target="cvs_checkout"
onClick="window.open('about:blank', 'cvs_checkout',
'resizeable=1,scrollbars=1')"
><b>download</b></a>)


<br>


<hr size=1 noshade>

<a name="rev1.2"></a>
<a name="HEAD"></a>

<a name="MAIN"></a>

Revision

<a href="/cgi-bin/viewcvs/*checkout*/grub/grub/stage2/xfs.h?rev=1.2" target="cvs_checkout"
onClick="window.open('about:blank', 'cvs_checkout',
'resizeable=1,scrollbars=1')"
><b>1.2</b></a>




/
<a href="xfs.h?rev=1.2&content-type=text/vnd.viewcvs-markup"><b>(view)</b></a>


- <a href="xfs.h?annotate=1.2">annotate</a>


- <a href="xfs.h?r1=1.2">[select for diffs]</a>



, <i>Sun Jan 18 19:47:18 2004 UTC</i> (4 weeks, 6 days ago) by <i>okuji</i>

<br>Branch:

<a href="xfs.h?only_with_tag=MAIN"><b>MAIN</b></a>



<br>CVS Tags:

<a href="xfs.h?only_with_tag=HEAD"><b>HEAD</b></a>





<br>Changes since <b>1.1: +106 -108 lines</b>




<br>Diff to <a href="xfs.h.diff?r1=1.1&r2=1.2">previous 1.1</a>







<pre>2004-01-18  Yoshinori K. Okuji  &lt;<a href="mailto:okuji@enbug.org">okuji@enbug.org</a>&gt;

	From Thomas Schwinge &lt;<a href="mailto:kischde@gmx.net">kischde@gmx.net</a>&gt;:
	* grub/Makefile.am (AM_CPPFLAGS): New variable.
	(AM_CFLAGS): Removed all cpp flags.

	* stage2/xfs.h (__int8_t): Renamed to ...
	(xfs_int8_t): ... this.
	(__uint8_t): Renamed to ...
	(xfs_uint8_t): ... this.
	(__int16_t): Renamed to ...
	(xfs_int16_t): ... this.
	(__uint16_t): Renamed to ...
	(xfs_uint16_t): ... this.
	(__int32_t): Renamed to ...
	(xfs_int32_t): ... this.
	(__uint32_t): Renamed to ...
	(xfs_uint32_t): ... this.
	(__int64_t): Renamed to ...
	(xfs_int64_t): ... this.
	(__uint64_t): Renamed to ...
	(xfs_uint64_t): ... this.
	All callers are changed.x

	From Egmont Koblinger &lt;<a href="mailto:egmont@uhulinux.hu">egmont@uhulinux.hu</a>&gt;:
	* util/grub-install.in: Support an install devices in GRUB's
	notation without parentheses.

	* docs/grub.texi (Installing GRUB using grub-install): Added an
	example of using grub-install without parentheses.
</pre>

<hr size=1 noshade>

<a name="rev1.1"></a>
<a name="release_0_93"></a>
<a name="release_0_92"></a>
<a name="release_0_91"></a>
<a name="prepare_0_91"></a>


Revision

<a href="/cgi-bin/viewcvs/*checkout*/grub/grub/stage2/xfs.h?rev=1.1" target="cvs_checkout"
onClick="window.open('about:blank', 'cvs_checkout',
'resizeable=1,scrollbars=1')"
><b>1.1</b></a>




/
<a href="xfs.h?rev=1.1&content-type=text/vnd.viewcvs-markup"><b>(view)</b></a>


- <a href="xfs.h?annotate=1.1">annotate</a>


- <a href="xfs.h?r1=1.1">[select for diffs]</a>



, <i>Sat Oct 27 16:04:25 2001 UTC</i> (2 years, 3 months ago) by <i>okuji</i>

<br>Branch:

<a href="xfs.h?only_with_tag=MAIN"><b>MAIN</b></a>



<br>CVS Tags:

<a href="xfs.h?only_with_tag=release_0_93"><b>release_0_93</b></a>,

<a href="xfs.h?only_with_tag=release_0_92"><b>release_0_92</b></a>,

<a href="xfs.h?only_with_tag=release_0_91"><b>release_0_91</b></a>,

<a href="xfs.h?only_with_tag=prepare_0_91"><b>prepare_0_91</b></a>










<pre>JFS and XFS support is added.
</pre>

<a name=diff></a>
<hr noshade>
This form allows you to request diffs between any two revisions of
a file. You may select a symbolic revision name using the selection
box or you may type in a numeric name using the type-in text box.
<p>
<form method="GET" action="xfs.h.diff" name="diff_select">
Diffs between
<select name="r1">
<option value="text" selected>Use Text Field</option>

<option value="1.1:release_0_93">release_0_93</option>

<option value="1.1:release_0_92">release_0_92</option>

<option value="1.1:release_0_91">release_0_91</option>

<option value="1.1.0.2:prepare_0_91">prepare_0_91</option>

<option value="0.1:MAIN">MAIN</option>

<option value="1.2:HEAD">HEAD</option>

</select>
<input type="TEXT" size="12" name="tr1" value="1.1"
onChange="document.diff_select.r1.selectedIndex=0">
and
<select name="r2">
<option value="text" selected>Use Text Field</option>

<option value="1.1:release_0_93">release_0_93</option>

<option value="1.1:release_0_92">release_0_92</option>

<option value="1.1:release_0_91">release_0_91</option>

<option value="1.1.0.2:prepare_0_91">prepare_0_91</option>

<option value="0.1:MAIN">MAIN</option>

<option value="1.2:HEAD">HEAD</option>

</select>
<input type="TEXT" size="12" name="tr2" value="1.2"
onChange="document.diff_select.r1.selectedIndex=0">
<br>Type of Diff should be a
<select name="diff_format" onchange="submit()">
<option value="h" selected>Colored Diff</option>
<option value="l" >Long Colored Diff</option>
<option value="u" >Unidiff</option>
<option value="c" >Context Diff</option>
<option value="s" >Side by Side</option>
</select>
<input type=submit value=" Get Diffs "></form>
<hr noshade>

<a name=branch></a>
<form method="GET" action="xfs.h">

View only Branch:
<select name="only_with_tag" onchange="submit()">
<option value="" selected>Show all branches</option>

<option value="prepare_0_91" >prepare_0_91</option>

<option value="MAIN" >MAIN</option>

</select>
<input type=submit value=" View Branch ">
</form>

<a name=logsort></a>
<form method="GET" action="xfs.h">

Sort log by:
<select name="logsort" onchange="submit()">
<option value="cvs" >Not sorted</option>
<option value="date" selected>Commit date</option>
<option value="rev" >Revision</option>
</select>
<input type=submit value=" Sort ">
</form>

<hr noshade>
<table width="100%" border=0 cellpadding=0 cellspacing=0><tr>
<td align=left><address>Send suggestions and report problems to the Savannah Hackers <a href="mailto:savannah-hackers@gnu.org">&lt;savannah-hackers@gnu.org&gt;</a>;</address></td>
<td align=right>
Powered by<br><a href="http://viewcvs.sourceforge.net/">ViewCVS 0.9.2</a>
</td></tr></table>
</body></html>

