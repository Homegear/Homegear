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
            $_SESSION['locale'] = array((array_key_exists('locale', $settings) ? $settings['locale'] : 'de-DE'));
        }

        $authorized = (isset($_SESSION["authorized"]) && $_SESSION["authorized"] === true && isset($_SESSION["user"]));
        if(!$authorized && $redirectToLogin)
        {
            header("Location: signin.php");
            die("unauthorized");
        }
        hg_set_user_privileges($_SESSION["user"]);
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
