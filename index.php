<!DOCTYPE html>
<html>
<head>
<script type="text/javascript" src="test.js"></script>
<style>
a {
	text-decoration: none;
	color: #2b3e50;
}
.player {
	color: #7c9ebd;
	position: fixed;
	bottom: 0;
	left: 0;
	right: 0;
	padding: 10px;
	min-height: 60px;
	max-height: 100px;
	overflow-y: auto;
	background: #2B3E50;
}
.player a {
	text-decoration: none;
	color: white;
}
</style>
</head>
<body>
<?php
$dir = 'ayfiles';
$files = glob($dir."/*.[Aa][Yy]");
foreach($files as $file) { ?>
	<div style="width: 22%; display: inline-block"><a href="javascript:" onclick="player.load(&quot;<?=htmlspecialchars(basename($file))?>&quot;, 0, 0, 1);"><?=htmlspecialchars(basename($file))?></a></div>
<?php
}
?>
<div class="padding" style="height: 40px;">&nbsp;</div>
<div id="player" class="player"></div>
</body>
</html>
