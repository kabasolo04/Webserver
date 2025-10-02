<html>
<body>
    <h1>php test</h1>
    <h4>
    <?php
    echo "GET: ";
    print_r($_GET);
    echo "\nPOST: ";
    print_r($_POST);

    echo 'query= ' . htmlspecialchars($_GET["potto"]) . ' !';
    ?>
    </h4>
    <?php phpinfo(); ?>
</body>
</html>


