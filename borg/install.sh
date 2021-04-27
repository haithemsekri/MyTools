apt install borgbackup


cd 
/media/backup
mkdir git-repos
df -h .

export BORG_PASSPHRASE='b'
mkdir /media/backup/git-repos
borg init --encryption=repokey /media/backup/git-repos
borg create --verbose --compression zstd,12 /media/backup/git-repos::initial /media/git-repos


borg create --verbose --compression lz4 /media/backup/git-repos::initial /media/git-repos
borg list /media/backup/git-repos

