

```


(def fid (fs/open "r" "/path/to/file"))

(def data (fs/read fid ... ))

(fs/close fid)


```