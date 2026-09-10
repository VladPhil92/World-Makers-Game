import { requireAuthorizedChild } from './authorization.mjs';

export const PRIVACY_OPERATIONS = Object.freeze([
  'export-child-data',
  'delete-child-data',
  'unlink-child-profile',
]);

export function buildPrivacyRequest({ family, parentId, childProfileId, operation, acknowledged }) {
  requireAuthorizedChild(family, parentId, childProfileId);
  if (!PRIVACY_OPERATIONS.includes(operation)) {
    throw new TypeError('Unsupported privacy operation.');
  }
  if (acknowledged !== true) {
    throw new TypeError('Explicit guardian acknowledgement is required.');
  }

  return {
    schemaVersion: 1,
    childProfileId,
    operation,
    requestedByRole: 'guardian',
    status: 'pending-backend-processing',
  };
}

export function validateLinkCode(value) {
  const normalized = String(value ?? '').trim().toUpperCase();
  if (!/^[A-Z0-9]{8}$/.test(normalized)) {
    throw new TypeError('Link code must contain exactly 8 letters or numbers.');
  }
  return normalized;
}
