export class AuthorizationError extends Error {
  constructor(message = 'Resource not available.') {
    super(message);
    this.name = 'AuthorizationError';
    this.code = 'not_authorized';
  }
}

function requireStableId(value, label) {
  if (typeof value !== 'string' || !/^[a-z0-9][a-z0-9._-]{2,95}$/.test(value)) {
    throw new TypeError(`${label} must be a stable non-PII identifier.`);
  }
  return value;
}

export function listAuthorizedChildren(family, parentId) {
  requireStableId(parentId, 'parentId');
  if (!family || !Array.isArray(family.children) || !Array.isArray(family.authorizedParentIds)) {
    throw new TypeError('Invalid family read model.');
  }
  if (!family.authorizedParentIds.includes(parentId)) {
    throw new AuthorizationError();
  }

  return family.children
    .filter((child) => Array.isArray(child.authorizedParentIds) && child.authorizedParentIds.includes(parentId))
    .map(({ childProfileId, displayAlias }) => ({ childProfileId, displayAlias }));
}

export function requireAuthorizedChild(family, parentId, childProfileId) {
  requireStableId(parentId, 'parentId');
  requireStableId(childProfileId, 'childProfileId');
  if (!family || !Array.isArray(family.children) || !family.authorizedParentIds?.includes(parentId)) {
    throw new AuthorizationError();
  }

  const child = family.children.find((candidate) => candidate.childProfileId === childProfileId);
  if (!child || !child.authorizedParentIds?.includes(parentId)) {
    // Intentionally identical for missing vs unauthorized child IDs to avoid enumeration.
    throw new AuthorizationError();
  }
  return child;
}

export function buildSessionReadModel(family, parentId) {
  return {
    authenticated: true,
    familyId: requireStableId(family.familyId, 'familyId'),
    parentRole: 'guardian',
    children: listAuthorizedChildren(family, parentId),
  };
}
