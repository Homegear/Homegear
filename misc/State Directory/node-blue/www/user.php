<?php
class User
{
    public function __construct()
    {
        session_start(array('name' => 'PHPSESSIDADMIN'));
    }

    public function checkAuth($redirectToLogin)
    {
        if (array_key_exists('SSL_CLIENT_VERIFY', $_SERVER) && $_SERVER['SSL_CLIENT_VERIFY'] == "SUCCESS" && !isset($_SESSION["authorized"])) {
            // CERT-Auth
            $settings = hg_get_user_metadata($_SERVER['SSL_CLIENT_S_DN_CN']);
            $_SESSION['authorized'] = true;
            $_SESSION['user'] = $_SERVER['SSL_CLIENT_S_DN_CN'];
            if (is_array($settings) && isset($settings['locale'])) $_SESSION['locale'] = array($settings['locale']);
        }

        if (array_key_exists('CLIENT_AUTHENTICATED', $_SERVER) && $_SERVER['CLIENT_AUTHENTICATED'] == "true" &&
            array_key_exists('CLIENT_VERIFIED_USERNAME', $_SERVER) && $_SERVER['CLIENT_VERIFIED_USERNAME'])
        {
            $settings = hg_get_user_metadata($_SERVER['CLIENT_VERIFIED_USERNAME']);
            $_SESSION['authorized'] = true;
            $_SESSION['user'] = $_SERVER['CLIENT_VERIFIED_USERNAME'];
            if (is_array($settings) && isset($settings['locale'])) $_SESSION['locale'] = array($settings['locale']);
        }

        $authorized = (isset($_SESSION["authorized"]) && $_SESSION["authorized"] === true && isset($_SESSION["user"]));
        if(!$authorized && $redirectToLogin)
        {
            header('Location: signin.php');
            die('unauthorized');
        }
        hg_set_user_privileges($_SESSION['user']);
        if(\Homegear\Homegear::checkServiceAccess('node-blue') !== true) return -2;

        if(!isset($_SESSION['locale']))
        {
            $settings = hg_get_user_metadata($_SESSION['user']);
            if (is_array($settings) && isset($settings['locale'])) $_SESSION['locale'] = array($settings['locale']);
        }

        return $authorized;
    }

    public function login($username, $password)
    {
        if(hg_auth($username, $password) === true)
        {
            hg_set_user_privileges($username);
            if(\Homegear\Homegear::checkServiceAccess("node-blue") !== true) return -2;
            $_SESSION["authorized"] = true;
            $_SESSION["user"] = $username;
            $settings = hg_get_user_metadata($username);
            if (is_array($settings) && isset($settings['locale'])) $_SESSION['locale'] = array($settings['locale']);
            return 0;
        }
        return -1;
    }

    public function logout()
    {
        if (ini_get("session.use_cookies"))
        {
            $params = session_get_cookie_params();
            setcookie(session_name(), '', time() - 42000,
                $params["path"], $params["domain"],
                $params["secure"], $params["httponly"]
            );
        }
        session_destroy();
    }
}
