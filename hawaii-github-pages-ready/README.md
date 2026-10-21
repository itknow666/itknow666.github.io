# hawaii-0 的小屋 — GitHub Pages 部署包

本包已经是静态网站，无需 npm install、npm run build，也没有 dist 目录。

1. 在电脑上解压 ZIP。不要将 ZIP 文件本身上传作为网站。
2. 将解压后的 index.html、assets/、documents/、projects/、.nojekyll 和 README.md 上传到 GitHub 仓库根目录。不要额外套一层 hawaii-0-blue-room-site 文件夹。
3. 打开仓库 Settings → Pages。
4. Source 选择 Deploy from a branch。
5. Branch 选择实际上传文件的分支（通常是 main），目录选择 /(root)，点击 Save。
6. 等待 Pages 部署完成，使用 Pages 页面显示的网站地址访问。

正确的仓库结构：
index.html
assets/
documents/
projects/
.nojekyll
README.md

本包仅调整压缩包层级、部署说明，并添加空的 .nojekyll；网页内容和资源保持原样。
