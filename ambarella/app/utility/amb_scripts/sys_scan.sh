#!/bin/bash
#########################################################################
# Version Info:
# Version: 1.2
# Date: 2014-06-24
# Comments: add kernal command line checking for UART.
#
# Version: 1.1
# Date: 2013-10-24
# Comments: initial version for release.
#########################################################################

#########################################################################
# Global Variables
RESULT_STR=""
PORT_STR=""

#########################################################################
# System Resouce Scan Functions
cpu_usage() {
    cpu_usage=$(ps aux|awk '{print $3}'|awk 'BEGIN {sum=0} {sum+=$1} END {print sum}')
    printf "\tCPU Usage: $cpu_usage %%\n"
}

process_mem_usage() {
    # $1 is the process name
    proc_mem_usage=$(ps aux|grep $1|grep -v "grep $1"|awk '{print $6}'|awk 'BEGIN {sum1=0} {sum1+=$1} END {print sum1/1024}')
    printf "\tProcess <$1> memory usage: $proc_mem_usage M\n"
}

sys_free_mem() {
    free_mem=$(free -m|awk '/Mem/{print $4}')
    printf "\tSystem free memory: $free_mem M\n"
}

root_disk_usage() {
    disk_usage=$(df|sed -n '/\/$/p'|awk '{print $5}'|sed 's/%//')
    if [ $disk_usage -ge 90 ]; then
        printf "\t[WARNING]:Root Mount Point Usage: $disk_usage >= 90%\n"
    else
        printf "\tRoot Mount Disk usage: $disk_usage %%\n"
    fi
}

audio_performance() {
    if [ -e "/proc/ambarella/performace" ]; then
        audio_hz=$(grep Audio /proc/ambarella/performance|awk -F: '{print $2}'|awk '{print $1}')
        printf "\tAudio Performance: $audio_hz HZ\n"
    elif [ -e "/proc/ambarella/clock" ]; then
        audio_hz=$(grep audio /proc/ambarella/clock|awk -F: '{print $2}'|awk '{print $1}')
        printf "\tAudio Performance: $audio_hz HZ\n"
    else
        printf "\t[WARNING]:Can't find clock file\n"
    fi
}

#########################################################################3
# Leak Scan Functions
passwd_scanner() {
    passwd_file=$1"/etc/passwd"
    printf "Scanning User Password File: ./etc/passwd ...\n"
    scan_file_right $passwd_file "-rw-r--r--"
    scan_plus_in_file $passwd_file
    scan_empty_passwd $passwd_file
    scan_zero_uid $passwd_file
    echo
}

login_pass_scanner() {
    printf "Scanning Login Password File: ./etc/login.defs ...\n"
    lg_ps=$1"/etc/login.defs"
    check_pass_len $lg_ps
    check_pass_aging $lg_ps
    echo
}

check_pass_len() {
    pass_len=$(grep ^PASS_MIN_LEN $1|awk '{print $2}')
    if [ "$pass_len" = "" ]; then
        print_unsafe "PASS_MIN_LEN is not specified\n"
    elif [ "$pass_len" -lt 6 ]; then
        print_unsafe "PASS_MIN_LEN is less then 6\n"
    else
        print_safe "PASS_MIN_LEN is OK\n"
    fi
}

check_pass_aging() {
    pass_aging=$(grep ^PASS_MAX_DAYS $1|awk '{ if ($1=="PASS_MAX_DAYS") {print $2} }')
    if [ "$pass_aging" = "" -o "$pass_aging" = "99999" ]; then
        print_unsafe "User password no aging set\n"
    else
        print_safe "Password aging is OK\n"
    fi
}

shadow_scanner() {
    shadow_file=$1"/etc/shadow"
    printf "Scanning File: ./etc/shadow ...\n"
    scan_file_right $shadow_file "-rw-----"
    scan_plus_in_file $shadow_file
    scan_empty_passwd $shadow_file
    echo
}

group_scanner() {
    group_file=$1"/etc/group"
    printf "Scanning Group File: ./etc/group ...\n"
    uniq_str=$(cat $group_file|grep -v '^#'|grep -v '^$'|awk -F: '{print $3}'|sort|uniq -d|tr -d '\n\r')
    if [ "$uniq_str" = "" ]; then
        print_safe "Non unique groud ID\n"
    else
        print_unsafe "Unique groud ID ($uniq_str) in group file\n"
    fi

    root_group=$(grep "^root" $group_file)
    root_group_user=$(echo $root_group|awk -F: '{print $4}')
    if [ -z "$root_group_user" ]
    then
        print_safe "NO user found in root group\n"
    else
        print_unsafe "User:($root_group_user) in root group\n"
    fi
    echo
}

service_scanner() {
    inittab_file=$1"/etc/inittab"
    systemd_sys_dir=$1"/usr/lib/systemd/system/"
    printf "Scanning System Service ...\n"

    printf "Telnet Service\n"
    check_service "telnet"

    printf "HTTP Service\n"
    check_service "httpd"
    if [ $? -eq 1 ]; then
        check_https_safty $1
    fi

    printf "FTP Service\n"
    check_service "ftp"

    printf "SSH Service\n"
    check_ssh_service $1
    echo
}


network_fs_scanner() {
    printf "Scanning Network Shared Filesystem ...\n"
    # Checking NFS
    if [ -e "$1/etc/exports" ]; then
        shared_folder=$(grep ^/ $1/etc/exports|awk '{print $1}')
        if [ -n "$shared_folder" ]; then
            print_unsafe "folder:($shared_folder) will be shared in NFS\n"
        else
            print_safe "No folder shared in NFS\n"
        fi
    else
        print_safe "NFS is not enabled\n"
    fi

    # Checking Samba
    if [ -e "$1/etc/samba/smb.conf" ]; then
        print_unsafe "Samba is build in rootfs\n"
    else
        print_safe "Samba is not build in\n"
    fi
    echo
}

mount_fs_scanner() {
    printf "Scanning Mount Point Config File /etc/fstab ...\n"
    msg_str=""
    fstab_file=$1"/etc/fstab"
    if [ -n "$(grep \"nfs\b\" $fstab_file)" ]; then
        msg_str="False"
        print_unsafe "NFS filesystem will be mounted\n"
    fi
    if [ -n "$(grep \"smbfs\b\" $fstab_file)" ]; then
        msg_str="False"
        print_unsafe "Samba filesystem will be mounted\n"
    fi
    if [ "$msg_str" = "" ]; then
        print_safe "Mount points are normal\n"
    fi
    echo
}

#####################################################3
# Kernel Scan
kernel_cmdline_scan() {
    printf "Scanning Kernel Boot Command Line ...\n"
    # If not parameter, then it's running on board.
    if [ "$1" == "" ]; then
        if [ "$(grep console=ttyS0 /proc/cmdline)" == "" ]; then
            print_safe "UART is disabled in kernel cmdline\n"
        else
            print_unsafe "console=ttyS0 should be removed or replaced by console=tty0\n"
        fi
    else
        kernel_cf=$1"/home/.config"
        if [ -z "$(grep "^AMBOOT_DEV_CMDLINE=\"console=ttyS0" $kernel_cf)" ]; then
            print_safe "UART is disabled in kernel cmdline\n"
        else
            print_unsafe "console=ttyS0 should be removed or replaced by console=tty0\n"
        fi
    fi
    echo
}
kernel_config_scanner() {
    printf "Scanning Kernel Config ...\n"
    if [ -e "/proc/config.gz" ]; then
        print_unsafe "config.gz is enabled\n"
    else
        print_safe "config.gz is disabled\n"
    fi
    if [ "$(cat /proc/sys/fs/suid_dumpable)" -eq "1" ]; then
        if [ "$(grep unlimited /etc/profile)" ]; then
            print_unsafe "Core dump is enabled\n"
        else
            print_unsafe "SUID programs core dump is enabled\n"
        fi
    else
        print_safe "core dump is not enabled\n"
    fi
    echo
}

#####################################################3
# On Board Scan functions
port_scanner() {
    printf "Scanning LISTEN Port ...\n"
    old_IFS=IFS
    IFS="
"
    for oneline in $(netstat -ntpl 2>&-|grep LISTEN|grep -v 127\.0\.)
    do
        PORT_STR=$PORT_STR$per_line$(echo $oneline|awk '{print "Program:<"$NF"> listen port:\e[31m"$4"\033[0m"}')"\n"
    done
    printf $PORT_STR
    IFS=old_IFS
    echo

}

check_ssh_service() {
    if [ -z "$(ls $1/etc/init.d/|grep sshd)" ]; then
        print_safe "Service (SSH) service not enabled\n"
        return
    fi

    printf "Service (SSH) will be run while booting\n"
    ssh_conf=$1"/etc/sshd_config"
    if [ -z "$(grep \"^PermitEmptyPasswords\ no\" $ssh_conf)" ]; then
        print_unsafe "<SSH>Empty password is permitted\n"
    fi

    if [ -z "$(grep \"^PermitRootLogin\ no\" $ssh_conf)" ]; then
        print_unsafe "<SSH>Root user is allowed to remote login\n"
    fi

    if [ -z "$(grep \"^Protocol\ 2\" $ssh_conf)" ]; then
        print_unsafe "<SSH>Protocol 1 is used as default, suggest 2.\n"
    fi

    if [ -z "$(grep ^LoginGraceTime $ssh_conf)" ]; then
        print_unsafe "<SSH>LoginGraceTime suggested to enable\n"
    fi

    if [ -z "$(grep ^MaxAuthTries $ssh_conf)" ]; then
        print_unsafe "<SSH>MaxAuthTries suggested to enable\n"
    fi

    if [ -z "$(grep ^ClientAliveInterval $ssh_conf)" ]; then
        print_unsafe "<SSH>ClientAliveInterval suggested to enable\n"
    fi

    if [ -z "$(grep ^ClientAliveCountMax $ssh_conf)" ]; then
        print_unsafe "<SSH>ClientAliveCountMax suggested to enable\n"
    fi

    if [ -z "$(grep \"^IgnoreRhosts\ yes\" $ssh_conf)" ]; then
        print_unsafe "<SSH>IgnoreRhosts suggested to enable\n"
    fi
}


check_service_file() {
    if [ "$(ls $systemd_sys_dir$1* 2>&-)" == "" ]; then
        print_safe "Service ($1) not in systemd system\n"
        return
    fi
    print_unsafe "Service ($1) will run in systemd system\n"
    return
}

check_service() {
    no_service="False"
    IFS_old=$IFS
    IFS="
"
    #Using systemd instead of init.d
    grep_str=$(grep "inittab is no longer used when using systemd" $inittab_file)
    if [ "$grep_str" != "" ]; then
        #Running on ambarella developing board.
        if [ -z "$(uname -a|grep Ambarella)" ]; then
            check_service_file $1
            return 0
        fi

        if [ "$(systemctl show $1*)" == "" ]; then
            print_safe "Service ($1) not in systemd system\n"
        else
            print_unsafe "Service ($1) will run in systemd system\n"
        fi
        return 0
    fi

    for LINE in $(grep $1 $inittab_file); do
        if [ -z "$(echo $LINE |awk -F# '{print $1}')" ]; then
            continue
        else
            print_unsafe "Service ($1) will RUN while booting\n"
            IFS=$IFS_old
            return 1
        fi
    done
    print_safe "Service ($1) will NOT run while booting\n"
    IFS=$IFS_old
    return 0
}

check_https_safty() {
    lighttpd_conf=$1"/etc/lighttpd.conf"
    userfile=$(grep userfile $lighttpd_conf|awk -F\" '{print $2}')
    if [ -z "$userfile" ]; then
        print_unsafe "Lighttpd user password file not specified\n"
        return
    fi
    if [ -e "$1$userfile" ]; then
        print_safe "Lighttpd user file existed\n"
    else
        print_unsafe "Lighttpd user file specified but no existed\n"
    fi
}

scan_file_right() {
    printf "0> File access right scanning\n"
    file_str=$(ls -l $1)
    file_mat=${file_str##"$2"}
    if [ "$file_str" = "$file_mat" ]; then
        print_unsafe "File in low safty access right.\n"
    else
        print_safe "File access right is safty.\n"
    fi
}

scan_plus_in_file() {
    printf "1> No '+' scanning\n"
    match_str=$(grep ^+: $1)
    if [ -z "$match_str" ]
    then
        print_safe "No '+' found.\n"
    else
        print_unsafe "'+' found in $match_str\n"
    fi
}

scan_empty_passwd() {
    printf "2> Empty password scanning\n"
    found_empty_pwd="false"
    cat $1|while read oneline
do
    passwd=$(echo $oneline| awk -F: '{print $2}')
    if [ -z "$passwd" ]
    then
        user=$(echo $oneline|awk -F: '{print $1}')
        print_unsafe "user($user) password is empty\n"
        found_empty_pwd="true"
    fi
done
    if [ $found_empty_pwd == "false" ]; then
        print_safe "No user with empty password in $1\n"
    fi
}

scan_zero_uid() {
    printf "3> No UID=0 scanning\n"
    UID_user=$(awk -F: '($3==0){print $1}' $1)
    if [ "$UID_user" = "root" ]; then
        print_safe "Only root has UID=0\n"
    elif [ -z "$UID_user" ]; then
        print_safe "No User has UID=0\n"
    else
        print_unsafe "User:$UID_user has UID=0\n"
    fi
}

#########################################################################3
# General Functions
print_safe() {
    OUTPUT_STR="[  \e[32mSafe\033[0m  ]: "$1
    printf "$OUTPUT_STR"
}

print_unsafe() {
    unsafe_str="[ \e[31mUnsafe\033[0m ]: "$1
    RESULT_STR=$RESULT_STR$unsafe_str
    printf "$unsafe_str"
}

help_opts() {
    printf "SYNOPSIS\n"
    printf "\t$1 [-p FAKE_ROOT_PATH]|[-h]\n"
    printf "OPTIONs:\n"
    printf "\t -P FAKE_ROOT_PATH :check files under fake root path.\n"
    printf "\t -h :show this help info.\n\n"
}

parse_opts() {
    case $1 in
    -p)
        if [ -z "$2" ]; then
            printf "Parameter error for option -p\n"
            help_opts $0
            exit 1
        fi
        return 1
        ;;
    -h)
        help_opts $0
        return 0
        ;;
    *)
        printf "Option Error.\n"
        help_opts $0
        exit 1
        ;;
    esac
}

check_run_on_board() {
    if [ -z "$(uname -a|grep Ambarella)" ]; then
        printf "$1: Not running script on board Now.\n"
        printf "Please specify the FAKE ROOT path by option '-p'\n\n"
        help_opts $1
        exit 1
    fi
}

sys_res_scanning() {
    printf "Scanning System Resource ...\n"
    cpu_usage
    sys_free_mem
    root_disk_usage
    audio_performance
    echo
}

fake_root_scanning() {
    passwd_scanner $1
    shadow_scanner $1
    group_scanner $1
    service_scanner $1
    network_fs_scanner $1
    mount_fs_scanner $1
    kernel_cmdline_scan $1
}

on_board_scanning() {
    fake_root_scanning

    port_scanner
    kernel_config_scanner

    sys_res_scanning
}

print_summary() {
    printf "=======================================================================\n"
    printf "Summary :\n"
    printf "$RESULT_STR\n"
    if [ "$PORT_STR" != "" ]; then
        printf "Listening Ports:\n"
        printf "$PORT_STR"
    fi
    echo
}

###################### Main Function ############################
if [ $# -gt 2 ]; then
    printf "$0: Wrong parameters!\n"
    help_opts $0
    exit 1
fi
echo
if [ $# -eq 0 ]; then
    check_run_on_board $0
    #If no parameter, suspect running script on board.
    on_board_scanning
else
    # Check on fake root fs.
    parse_opts $@
    if [ $? -eq 1 ]; then
        fake_root_scanning $2
    fi
fi

print_summary
exit 0
