<?php
class User
{
    public function __construct()
    {
        session_start(array('name' => 'PHPSESSIDADMIN'));
    }

    public function checkAuth($redirectToLogin)
    {
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
