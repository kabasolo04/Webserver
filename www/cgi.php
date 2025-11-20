<?php
// Ensure the CGI emits a proper Content-Type header when run under php-cgi
header('Content-Type: text/html');
?>
<html>
<body>
    <h1>php test</h1>
    <h4>
    <?php
    echo "GET: ";
    print_r($_GET);
    echo "\nPOST: ";
    print_r($_POST);

    if (isset($_GET["potto"])) echo 'query= ' . htmlspecialchars($_GET["potto"]) . ' !';
    if (isset($_POST["potto"])) echo ' post= ' . htmlspecialchars($_POST["potto"]) . ' !';
    ?>
    </h4>
    <?php // phpinfo(); ?>
</body>
</html>


