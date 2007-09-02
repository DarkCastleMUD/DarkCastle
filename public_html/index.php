<?php
{
  require('common.inc.php');

  if (!authenticated()) {
    header('location: login.php');
    exit;
  }
}
?>
<html>
<head><title>Main Menu</title></head>
<body>
<table bgcolor="#CCCCCC" border="0" width="100%">
<tr bgcolor="#AAAAAA"><td width="100%">Username: <?=$_SESSION['login'];?></td><td nowrap><strong>Main Menu</strong></td><td nowrap><a href="logout.php">Logout</a></td></tr>
<tr height="400"><td colspan="3" valign="top">
<div align="center"><a href="editor.php">Online Editor</a></div>
</td></tr>
</table>
</body>