<?php
{
  require('common.inc.php');
  if (!authenticated()) {
    header('location: login.php');
    exit;
  }

  $type = get_editor_type();
  if ($type === 'Obj or Mob program') {
    if (isset($_POST['contents'])) {
      $contents = editor($_POST['contents']);
      $type = get_editor_type();
    } else {
      $contents = editor(null);
    }
  }
}
?>
<html>
<head><title>editor</title></head>
<body>
<table bgcolor="#CCCCCC" border="0" width="100%">
<tr bgcolor="#AAAAAA"><td width="100%">Username: <?=$_SESSION['login'];?></td><td nowrap><a href="index.php">Main Menu</a></td><td nowrap><a href="logout.php">Logout</a></td></tr>
<tr><td colspan="3">
<?php if ($type == 'plr_editor_dc') { ?>
<p>Your current editor mode is in game. To set your editor to web type the following:</p> editor web
<?php } else if ($type === 'unknown') { ?>
<p>No current edit mode has been entered.</p>
Example of commands to enter an edit mode:<br>
procedit 1 command 1<br>
opedit 1 command 1<br>
<br>
Click <a href="editor.php">here</a> once done.
<?php } else { ?>
<form name="editor" method="POST">

<table border=0 cellspacing="3" align="center">
<th colspan="2">Editor Type:<?php print_r($type);?></th>
<tr><td><textarea name="contents" rows=25 cols=80><?php echo $contents;?></textarea></td></tr>
<tr><td align="center"><input type="submit" name="submit" value="submit"></td></tr>
</table>

</form>
<?php } ?>
</td></tr>
</table>
</body>
</html>