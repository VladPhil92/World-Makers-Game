# World Makers — Original Visual Masters v3

This directory is reserved for the four user-supplied PNG masters approved on 2026-09-14 for the public World Makers website refresh.

## Preservation contract

The PNG files must be committed **byte-for-byte** from the supplied originals. Do not recompress, re-encode, resize, crop, convert to WebP/AVIF, run an image optimizer, strip metadata, or pass them through the Next.js image optimizer. The directory-scoped `.gitattributes` disables the repository-wide Git LFS filter so `raw.githubusercontent.com` can return the literal PNG bytes.

The website integration is allowed to scale the images responsively with CSS, but the stored and transmitted source bytes must remain unchanged.

## Required masters

| Repository filename | Supplied filename | Role | Dimensions | Bytes | SHA-256 | Git blob SHA |
|---|---|---|---:|---:|---|---|
| `Portada sin letras(4).png` | `Portada sin letras(4).png` | Clean public hero, without baked-in letters or logos | 1672×941 | 3,741,981 | `8626983c068662f8b014e84e10afdcb9a862bc6103989dee0042a52b53b33220` | `d4cbc1ee9af3ea712074b4f20cefb69c432c89a9` |
| `Antes y Después(1).png` | `Antes y Después(1).png` | Environment transformation / restoration visual | 1536×1024 | 4,041,120 | `0e43dd5b6b8307d09d914e8ea422cc7675cb5998813e534b616880d7f5bde658` | `14dc2b43fbfc34fe7c9525e8b83ea6743ce4d5e1` |
| `Construcción Ecológica(1).png` | `Construcción Ecológica(1).png` | Sustainable construction visual | 1536×1024 | 3,769,062 | `718707c2e867b41822501385aa1178847854d2a1e4993e644efc10ede27d9ca6` | `25d385ca57eaeb10892780577dd8a15b81821539` |
| `Experimento(1).png` | `Experimento(1).png` | Science / experimentation visual | 1536×1024 | 3,470,760 | `00c6454629a7dbf9f138f1779d28c1b238624d0163604ef0617077bfe1efd468` | `b3825d4af1c76f39c7ab5206e26420bfd69d8e78` |

## Public-page placement

- `Portada sin letras(4).png`: first viewport hero. All public title, copy, navigation and CTA elements remain semantic HTML overlays; none are baked into the image.
- `Antes y Después(1).png`: transformation / environmental consequence story.
- `Experimento(1).png`: experimentation and learn-by-doing story.
- `Construcción Ecológica(1).png`: building and sustainability story.

## Acceptance rule

A file is accepted only if both its SHA-256 and Git blob SHA match the table above. Any mismatch means the asset was modified and must not be promoted as the v3 master.
