const nativeScheme = 'worldmakers';
const nativeHost = 'launch';
const defaultProtocol = 'worldmakers-launch-v1';

export function createNativeLaunchUri(context, protocol = defaultProtocol) {
  if (!context || typeof context !== 'object' || Array.isArray(context)) throw new TypeError('Launch context is required.');
  if (!protocol || typeof protocol !== 'string') throw new TypeError('Launch protocol is required.');
  const payload = Buffer.from(JSON.stringify(context), 'utf8').toString('base64url');
  const params = new URLSearchParams({ protocol, payload });
  return `${nativeScheme}://${nativeHost}?${params.toString()}`;
}

export function decodeNativeLaunchUri(uri) {
  const parsed = new URL(uri);
  if (parsed.protocol !== `${nativeScheme}:` || parsed.hostname !== nativeHost) throw new TypeError('Unsupported World Makers launch URI.');
  const protocol = parsed.searchParams.get('protocol');
  const payload = parsed.searchParams.get('payload');
  if (!protocol || !payload) throw new TypeError('Incomplete World Makers launch URI.');
  const context = JSON.parse(Buffer.from(payload, 'base64url').toString('utf8'));
  return { protocol, context };
}
