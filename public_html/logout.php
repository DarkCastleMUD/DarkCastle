<?php
{
  require('common.inc.php');

  unset($_SESSION['status'], $_SESSION['login'], $_SESSION['password']);

  header('location: login.php');
  exit;
}
?>