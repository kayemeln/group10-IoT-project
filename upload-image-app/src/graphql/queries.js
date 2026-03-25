/* eslint-disable */
// this is an auto generated file. This will be overwritten

export const getParty = /* GraphQL */ `
  query GetParty($id: ID!) {
    getParty(id: $id) {
      id
      partyName
      hostEmail
      inviteToken
      password
      uploads {
        nextToken
        __typename
      }
      createdAt
      updatedAt
      __typename
    }
  }
`;
export const listParties = /* GraphQL */ `
  query ListParties(
    $filter: ModelPartyFilterInput
    $limit: Int
    $nextToken: String
  ) {
    listParties(filter: $filter, limit: $limit, nextToken: $nextToken) {
      items {
        id
        partyName
        hostEmail
        inviteToken
        password
        createdAt
        updatedAt
        __typename
      }
      nextToken
      __typename
    }
  }
`;
export const getGuestUpload = /* GraphQL */ `
  query GetGuestUpload($id: ID!) {
    getGuestUpload(id: $id) {
      id
      partyId
      guestName
      photoPaths
      uploadedAt
      createdAt
      updatedAt
      __typename
    }
  }
`;
export const listGuestUploads = /* GraphQL */ `
  query ListGuestUploads(
    $filter: ModelGuestUploadFilterInput
    $limit: Int
    $nextToken: String
  ) {
    listGuestUploads(filter: $filter, limit: $limit, nextToken: $nextToken) {
      items {
        id
        partyId
        guestName
        photoPaths
        uploadedAt
        createdAt
        updatedAt
        __typename
      }
      nextToken
      __typename
    }
  }
`;
export const guestUploadsByParty = /* GraphQL */ `
  query GuestUploadsByParty(
    $partyId: ID!
    $uploadedAt: ModelStringKeyConditionInput
    $sortDirection: ModelSortDirection
    $filter: ModelGuestUploadFilterInput
    $limit: Int
    $nextToken: String
  ) {
    guestUploadsByParty(
      partyId: $partyId
      uploadedAt: $uploadedAt
      sortDirection: $sortDirection
      filter: $filter
      limit: $limit
      nextToken: $nextToken
    ) {
      items {
        id
        partyId
        guestName
        photoPaths
        uploadedAt
        createdAt
        updatedAt
        __typename
      }
      nextToken
      __typename
    }
  }
`;
