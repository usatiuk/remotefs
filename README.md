# RemoteFS

Simple server-client remote filesystem

## How to use

1. Create an SSL certificate for the server `openssl req  -nodes -new -x509  -keyout key.pem -out cert.pem`
1. Create a password hash for the user `echo -n "password" | openssl dgst -sha256`
1. Copy the password hash into a `users` file
1. Start the server `remotefs --mode:server --path:<path to filesystem root> --users_path:users`
1. Copy `cert.pem` to the client working directory
1. Connect with the client `remotefs --mode:client --username:<username> --password:<password> --path:<mount point>`

## Advanced options

Various options can be overridden using `--<option>:<value>` syntax

- `ip` - ip server will listen on, or client will connect to, default is `127.0.0.1`
- `port` - port server will listen on, or client will connect to, default is `42069`
- `default_log_level` - default logging level, 1 is least verbose, 4 is most verbose, default is `2`
- `timeout` - timeout, default is 30 (seconds)
- `ca_path` - path to SSL certificate, default is `cert.pem`
- `pk_path` - path to SSL private key (for server), default is `key.pem`
- `mode` - server or client, default is `server`
- `path` - filesystem root to server or mountpoint for server and client (default is empty)
- `acl_path` - path for an ACL config file, default is empty (and everything is allowed)
- `users_path` - path for file with user passwords (default is `users`)
- `username` - username to use for client
- `password` - password to use for client

Example with some of these options:

```
remotefs --mode:client  --default_log_level:3 --timeout:10 --username:user2 --password:password2 --path:"${HOME}/remotefs-test/mount2" --ip:192.168.88.20
```

## ACL and users configuration

Username passwords can be configured in a `users` file, with a simple format

```
<username> <sha256 password hash>
```

For example, two users with `password` password

```
username 5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8
username2 5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8
```

ACL can be configured in an optionally specified acl file, with format:

```
<path prefix> <allowed users, separated by string>
```

If user list is empty, then everyone is allowed. If there are multiple entries with same prefix,
the more specific one has higher priority.

Example:

```
/ user1
/A user1 user2
/B user2
/C
```

In this example, all files will be accessible "by default" only by user1,
with files in directory `/A` by user1 and user2, in directory `/B` by user2, and in
directory `/C` by everyone.