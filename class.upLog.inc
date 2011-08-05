<?php

/**
 * A KISS style php logging class
 * Automatically deletes log files after a period of time
 *
 * @author Uli Pahlke <github@matrix-tools.de>
 * @link http://www.matrix-tools.de
 * @version 0.1
 */

/** Usage
$log = new upLog('logs/');
$log->deleteAfterDays = 7;
$log->setFilename = 'error';
$log->writeLog('Log error string to error.log...');
$log->writeLog('Log debug string to debug.log...','debug');
*/

class upLog
{
    public $deleteAfterDays = 30;
    public $allowHtml = 0;
    public $setFilename;
    public $setTriggerError = 0;

    private $setDirname;
    private $setSuffix = 'log';
    private $defaultFileName = 'default';
    private $splitter = ' - ';
    private $origDir;

    /* public */

    public function __construct($dir = '')
    {
        if (!empty($dir)) {
            $this->sorigDir = $this->_trim($dir);
            $this->setDirname = $this->_modifyTrailingSlashes($this->sorigDir);
            $this->setDirname = $this->_prepareDirPath($this->setDirname);
            if (!empty($this->setDirname)) {
                $this->setDirname = $this->setDirname . DIRECTORY_SEPARATOR;
            }
        }
    }

    public function writeLog($str, $explizitFileName = "")
    {
        if (!$this->_mbIsUtf8($str)) {
            $str = utf8_encode($str);
        }
        $tmpSetFilename = '';
        if (!empty($this->setFilename)) {
            $tmpSetFilename = $this->setFilename;
        }
        if (!empty($explizitFileName)) {
            $this->setFilename = $explizitFileName;
        }
        if (!is_dir($this->setDirname)) {
            $this->_triggerError("No valid log dir! [ " . $this->sorigDir . " ]", __FILE__, __LINE__);
        } elseif (is_dir($this->setDirname) && !is_writable($this->setDirname)) {
            $this->_triggerError("No writable log dir! [ " . $this->sorigDir . " ]", __FILE__, __LINE__);
        } else {
            $logText = $this->_getDate() . $this->splitter;
            $logText .= $this->_getIP() . $this->splitter;
            $logText .= $this->_replace($str);
            $this->_writeFile($logText, $this->setDirname . $this->_createFileName());
            $this->_deleteFiles();
        }
        if (!empty($tmpSetFilename)) {
            $this->_setFilename = $tmpSetFilename;
        }
    }

    public function forceDeleteFiles($forcedDir, $suffix)
    {
        $forcedDir = $this->_modifyTrailingSlashes($forcedDir);
        $forcedDir = $this->_prepareDirPath($forcedDir);
        if (!empty($forcedDir)) {
            $forcedDir = $forcedDir . DIRECTORY_SEPARATOR;
        }
        $this->_deleteFiles($forcedDir, $suffix);
    }

    public function _deleteFiles($forcedDir = '', $suffix = '')
    {
        if (!empty($forcedDir)) {
            $dirToChk = $forcedDir;
        } else {
            $dirToChk = $this->setDirname;
        }
        if (empty($suffix)) {
            $suffix = $this->_suffix();
        }
        if (!file_exists($dirToChk . $this->_fileDate() . '.checked'))
        {
            // global overwrite
            if (defined('DELETE_LOGS_AFTER_DAYS') && DELETE_LOGS_AFTER_DAYS > 0) {
                $this->deleteAfterDays = DELETE_LOGS_AFTER_DAYS;
            }
            $logFilesArr = $this->_getLogFiles($forcedDir, $suffix);
            if (!empty($logFilesArr) && is_array($logFilesArr)) {
                foreach ($logFilesArr AS $k => $v) {
                    $fileToCheck = substr(basename($v), 0, 8);
                    if ($this->_fileDate(time() - (86400 * $this->deleteAfterDays)) >= $fileToCheck) {
                        if (file_exists($v)) {
                            $this->_unlink($v);
                        }
                    }
                }
                $this->_writeFile(1, $dirToChk . $this->_fileDate() . '.checked');
                foreach (glob($dirToChk . "*.checked") as $file) {
                    $fileToCheck = substr(basename($file), 0, 8);
                    if ($this->_fileDate(time() - 86400) >= $fileToCheck) {
                        if (file_exists($file)) {
                            $this->_unlink($file);
                        }
                    }
                }
            }
        }
    }

    public function _getLogFiles($forcedDir = '', $suffix = '')
    {
        if (!empty($forcedDir)) {
            $dirToChk = $forcedDir;
        } else {
            $dirToChk = $this->setDirname;
        }
        if (empty($suffix)) {
            $suffix = $this->_suffix();
        }
        if (is_dir($dirToChk)) {
            $files = array();
            foreach (glob($dirToChk . "*" . $suffix) as $file) {
                $files[] = $this->_modifyBackSlashes($file);
            }
        }
        return $files;
    }

    /* private */

    private function _getIP($getHostByAddr = '')
    {
        if (getenv('HTTP_CLIENT_IP')) {
            $ip = getenv('HTTP_CLIENT_IP');
        } elseif (getenv('HTTP_X_FORWARDED_FOR')) {
            $ip = getenv('HTTP_X_FORWARDED_FOR');
        } elseif (getenv('REMOTE_ADDR')) {
            $ip = getenv('REMOTE_ADDR');
        } else {
            $ip = "NOIP!";
        }
        if ($getHostByAddr == true) {
            return getHostByAddr($ip);
        } else {
            if (strstr($ip, ',')) {
                $tmpExpl = explode(",", $ip);
                $ip = $this->_trim($tmpExpl[0]);
            }
            return $ip;
        }
    }

    private function _createFileName()
    {
        if (!empty($this->setFilename)) {
            $this->setFilename = preg_replace("~^[_]+~", "", $this->_trim($this->setFilename));
            $fn = $this->_fileDate() . '_' . $this->_trim($this->setFilename) . $this->_suffix();
        } else {
            $fn = $this->_fileDate() . '_' . $this->_trim($this->defaultFileName) . $this->_suffix();
        }
        return $fn;
    }

    private function _getDate()
    {
        return date("Y-m-d H:i:s");
    }

    private function _fileDate($time = '')
    {
        if (!empty($time)) {
            return date("Ymd", $time);
        } else {
            return date("Ymd");
        }
    }

    private function _modifyBackSlashes($path)
    {
        return preg_replace("~[\\\]~", "/", $path);
    }

    private function _modifyTrailingSlashes($path)
    {
        $path = $this->_modifyBackSlashes($path);
        return preg_replace("~/$~", "", $this->_trim($path)) . '/';
    }

    private function _prepareDirPath($dir)
    {
        return realpath($this->_trim($dir));
    }

    private function _trim($str)
    {
        return trim($str);
    }

    private function _writeFile($str, $path)
    {
        $fp = fopen($path, 'a+b');
        flock($fp, 2);
        fwrite($fp, $str . "\n");
        flock($fp, 3);
        fclose($fp);
    }

    private function _unlink($file)
    {
        @unlink($this->_trim($file));
    }

    private function _replace($str)
    {
        if (is_array($str)) {
            $str = print_r($str, 1);
        }
        if ($this->allowHtml < 1) {
            $str = strip_tags($this->_trim($str));
        }
        $str = preg_replace("~[\n]+|[\r]+|[\t]+~", " ", $this->_trim($str));
        return preg_replace("~[ ]{2,}~", " ", $str);
    }

    public function _is_utf8($str)
    {
        $strlen = strlen($str);
        for($i = 0; $i < $strlen; $i++) {
            $ord = ord($str[$i]);
            if ($ord < 0x80) continue;
            elseif (($ord&0xE0) === 0xC0 && $ord > 0xC1) $n = 1;
            elseif (($ord&0xF0) === 0xE0) $n = 2;
            elseif (($ord&0xF8) === 0xF0 && $ord < 0xF5) $n = 3;
            else return 0;
            for($c = 0; $c < $n; $c++)
            if (++$i === $strlen || (ord($str[$i])&0xC0) !== 0x80)
                return 0;
        }
        return 1;
    }

    public function _mbIsUtf8($str)
    {
        if (mb_detect_encoding($str, 'UTF-8, ISO-8859-1') === 'UTF-8') {
            return 1;
        } else {
            return $this->_is_utf8($str);
        }
    }

    private function _suffix()
    {
        $suffix = $this->_trim(preg_replace("~[^0-9a-z]~", "", strtolower($this->setSuffix)));
        if (empty($suffix)) {
            $suffix = 'log';
        }
        return '.' . $suffix;
    }

    private function _triggerError($errMsg, $file = '', $line = '')
    {
        if (!empty($this->setTriggerError)) {
            exit($errMsg . " (File: " . basename($file) . " / Line: " . $line . ")");
        }
    }
}

?>