create database if not exists bruconbadges;



use bruconbadges;

drop table if exists badges;
drop table if exists alc_reading;
drop table if exists quizz_q;
drop table if exists quizz_r;



create table badges (
mac varchar(17),
username varchar(256),
time_enroll datetime not null,
constraint PK_mac primary key (mac)
);

create table alc_reading (
mac varchar(17),
time_reading datetime not null,
reading int,
 primary key (mac,time_reading)
);

create table quizz_r (
id int,
r varchar(256),
 primary key (id)
);

create table quizz_q (
id int,
q varchar(256),
r0 int,
r1 int,
r2 int,
 primary key (id),
 foreign key (r0) references quizz_r (id),
 foreign key (r1) references quizz_r (id),
 foreign key (r2) references quizz_r (id)
);

GRANT SELECT on bruconbadges.* TO 'bruconbadges_php'@'localhost' IDENTIFIED by 'xxxxxxxxx';
GRANT INSERT on bruconbadges.badges TO 'bruconbadges_php'@'localhost';
GRANT UPDATE (username) on bruconbadges.badges TO 'bruconbadges_php'@'localhost';
GRANT INSERT on bruconbadges.alc_reading TO 'bruconbadges_php'@'localhost';
