#
# (C) Tenable Network Security
#
# This plugin text was extracted from SuSE Security Advisory SUSE-SA:2003:031
#


if ( ! defined_func("bn_random") ) exit(0);
if(description)
{
 script_id(13800);
 script_version ("$Revision: 1.5 $");
 script_cve_id("CVE-2003-0252");
 
 name["english"] = "SUSE-SA:2003:031: nfs-utils";
 
 script_name(english:name["english"]);
 
 desc["english"] = "
The remote host is missing the patch for the advisory SUSE-SA:2003:031 (nfs-utils).


The nfs-utils package contains various programs to offer and manage
certain RPC services such as the rpc.mountd.
iSEC Security Research has reported an off-by-one bug in the xlog()
function used by the rpc.mountd. It is possible for remote attackers
to use this off-by-one overflow to execute arbitrary code as root.
Some of the products listed above seem not vulnerable to this
one byte overflow due to the stack alignment generated by the compiler
during the build. Nevertheless, since there is no easy workaround except
shutting down the RPC services, an update is strongly recommended for
every product listed above.

This update needs to be applied to both NFS servers and clients,
as the vulnerable function is used by mountd and statd.

You can either restart these services manually, or use the
corresponding init scripts:

'rcnfsserver restart'           (for the NFS server)
'rcnfs restart'                 (for the NFS client)

Depending on your setup, it may be less disruptive to restart
the two daemons manually.

Please download the update package for your distribution and verify its
integrity by the methods listed in section 3) of this announcement.
Then, install the package using the command 'rpm -Fhv file.rpm' to apply
the update.

Solution : http://www.suse.de/security/2003_031_nfs_utils.html
Risk factor : High";



 script_description(english:desc["english"]);
 
 summary["english"] = "Check for the version of the nfs-utils package";
 script_summary(english:summary["english"]);
 
 script_category(ACT_GATHER_INFO);
 
 script_copyright(english:"This script is Copyright (C) 2004 Tenable Network Security");
 family["english"] = "SuSE Local Security Checks";
 script_family(english:family["english"]);
 
 script_dependencies("ssh_get_info.nasl");
 script_require_keys("Host/SuSE/rpm-list");
 exit(0);
}

include("rpm.inc");
if ( rpm_check( reference:"nfs-utils-0.3.1-112", release:"SUSE7.2") )
{
 security_hole(0);
 exit(0);
}
if ( rpm_check( reference:"nfs-utils-0.3.1-111", release:"SUSE7.3") )
{
 security_hole(0);
 exit(0);
}
if ( rpm_check( reference:"nfs-utils-0.3.3-178", release:"SUSE8.0") )
{
 security_hole(0);
 exit(0);
}
if ( rpm_check( reference:"nfs-utils-1.0.1-109", release:"SUSE8.1") )
{
 security_hole(0);
 exit(0);
}
if ( rpm_check( reference:"nfs-utils-1.0.1-109", release:"SUSE8.2") )
{
 security_hole(0);
 exit(0);
}
if (rpm_exists(rpm:"nfs-utils-", release:"SUSE7.2")
 || rpm_exists(rpm:"nfs-utils-", release:"SUSE7.3")
 || rpm_exists(rpm:"nfs-utils-", release:"SUSE8.0")
 || rpm_exists(rpm:"nfs-utils-", release:"SUSE8.1")
 || rpm_exists(rpm:"nfs-utils-", release:"SUSE8.2") )
{
 set_kb_item(name:"CVE-2003-0252", value:TRUE);
}