<?php
{
  require('common.inc.php');

  // New login
  if (!empty($_POST['submit']) && $_POST['submit'] == 'login') {
    if (isset($_POST['login'])) {
      $_SESSION['login'] = $_POST['login'];
    }
    if (isset($_POST['password'])) {
      $_SESSION['password'] = $_POST['password'];
    }
  }

  if (authenticated()) {
    header('location: editor.php');
    exit;
  }
}
?>
<html>
<head><title>Login</title></head>
<body>
<form name="login" method="POST" action="login.php">
<table border=0 cellspacing="3" bgcolor="#CCCCCC" align="center">
<tr><td align="right">Login</td><td><input name="login" type="text"></td></tr>
<tr><td align="right">Password</td><td><input name="password" type="password"></td></tr>
<tr><td>&nbsp;</td><td><font color="red"><?php if (isset($_SESSION['status'])) echo $_SESSION['status'];?></font></td></tr>
<tr><td colspan="2" align="center"><input type="submit" name="submit" value="login"></td></tr>
</table>
</form>
</body>
</html>