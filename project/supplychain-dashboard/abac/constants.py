"""
Defines Enums and policy definitions for Attribute-Based Access Control (ABAC).
This is the one source of truth for ABAC configuration.
"""

from enum import Enum
from typing import Dict, List, Tuple

# ----------------------------------------
# Action Enum
# ----------------------------------------
class Action(Enum):
    CREATE_PRODUCT = "create_product"
    VIEW_PRODUCT = "view_product"
    DELETE_PRODUCT = "delete_product"

    CREATE_ORDER = "create_order"
    VIEW_ORDER = "view_order"

    CREATE_EVENT = "create_event"
    VIEW_EVENT = "view_event"

    CREATE_CUSTODY_TRANSFER = "create_custody_transfer"
    VIEW_CUSTODY_TRANSFER = "view_custody_transfer"

    VIEW_REQUIREMENT = "view_requirement"

    ALL = "*"

    def __str__(self):
        return self.value


# ----------------------------------------
# Subject Attribute Enum
# ----------------------------------------
class SubjectAttribute(Enum):
    COMPANY_TYPE = "company_type"
    COMPANY_ID = "company_id"
    SCANNED_PRODUCTS = "scanned_products"
    IS_SUPERUSER = "is_superuser"

    def __str__(self):
        return self.value


# ----------------------------------------
# Resource Attribute Enum
# ----------------------------------------
class ResourceAttribute(Enum):
    SUPPLIER_COMPANY_ID = "supplier_company_id"
    RECEIVER_COMPANY_ID = "receiver_company_id"
    PRODUCT_OWNER_ID = "product_owner_id"
    PRODUCT_ORDER_ID = "product_order_id"
    FROM_COMPANY_ID = "from_company_id"
    TO_COMPANY_ID = "to_company_id"
    ORDER_ID = "order_id"
    REQUIREMENT_ID = "requirement_id"
    PRODUCT_KEY = "product_key"

    def __str__(self):
        return self.value


# ----------------------------------------
# Policy Name Enum
# ----------------------------------------
class PolicyName(Enum):
    SUPPLIER_VIEW_ORDERS_THEY_RECEIVE = "Supplier_View_Orders_They_Receive"
    SUPPLIER_RECORD_SHIPMENT_EVENT = "Supplier_Record_Shipment_Event"
    MANUFACTURER_CREATE_ORDERS = "Manufacturer_Create_Orders"
    MANUFACTURER_VIEW_ORDERS_THEY_SUPPLY = "Manufacturer_View_Orders_They_Supply"
    MANUFACTURER_RECORD_MANUFACTURE_EVENT = "Manufacturer_Record_Manufacture_Event"
    DISTRIBUTOR_VIEW_CUSTODY_TRANSFERS = "Distributor_View_Custody_Transfers"
    DISTRIBUTOR_RECORD_CUSTODY_TRANSFER = "Distributor_Record_Custody_Transfer"
    RETAILER_VIEW_ORDERS_THEY_RECEIVE = "Retailer_View_Orders_They_Receive"
    RETAILER_RECORD_SALE_EVENT = "Retailer_Record_Sale_Event"
    RETAILER_RECORD_CUSTODY_TRANSFER = "Retailer_Record_Custody_Transfer"
    CONSUMER_VIEW_PRODUCT_PASSPORT = "Consumer_View_Product_Passport"
    ADMIN_FULL_ACCESS = "Admin_Full_Access"

    def __str__(self):
        return self.value


# ----------------------------------------
# Effect Enum
# ----------------------------------------
class Effect(Enum):
    ALLOW = "allow"
    DENY = "deny"

    def __str__(self):
        return self.value


# ----------------------------------------
# Policy Definitions
# ----------------------------------------
# Mapping from PolicyName to its definition.
# Each definition is a dict with keys:
#   description: str
#   effect: Effect
#   subject_attributes: List[Tuple[SubjectAttribute, str]]
#   resource_attributes: List[Tuple[ResourceAttribute, str]]
#   actions: List[Action]
POLICY_DEFINITIONS: Dict[PolicyName, Dict] = {
    PolicyName.SUPPLIER_VIEW_ORDERS_THEY_RECEIVE: {
        "description": "Suppliers can see orders placed TO them by manufacturers.",
        "effect": Effect.ALLOW,
        "subject_attributes": [
            (SubjectAttribute.COMPANY_TYPE, "supplier"),
        ],
        "resource_attributes": [
            (ResourceAttribute.RECEIVER_COMPANY_ID, "${subject.company_id}"),
        ],
        "actions": [
            Action.VIEW_ORDER,
        ],
    },
    PolicyName.SUPPLIER_RECORD_SHIPMENT_EVENT: {
        "description": "Suppliers can record shipment events for orders they supply.",
        "effect": Effect.ALLOW,
        "subject_attributes": [
            (SubjectAttribute.COMPANY_TYPE, "supplier"),
        ],
        "resource_attributes": [
            (ResourceAttribute.SUPPLIER_COMPANY_ID, "${subject.company_id}"),
        ],
        "actions": [
            Action.CREATE_EVENT,
        ],
    },
    PolicyName.MANUFACTURER_CREATE_ORDERS: {
        "description": "Manufacturers can place orders to suppliers.",
        "effect": Effect.ALLOW,
        "subject_attributes": [
            (SubjectAttribute.COMPANY_TYPE, "manufacturer"),
        ],
        "resource_attributes": [],
        "actions": [
            Action.CREATE_ORDER,
        ],
    },
    PolicyName.MANUFACTURER_VIEW_ORDERS_THEY_SUPPLY: {
        "description": "Manufacturers can view orders they have placed as suppliers to retailers.",
        "effect": Effect.ALLOW,
        "subject_attributes": [
            (SubjectAttribute.COMPANY_TYPE, "manufacturer"),
        ],
        "resource_attributes": [
            (ResourceAttribute.SUPPLIER_COMPANY_ID, "${subject.company_id}"),
        ],
        "actions": [
            Action.VIEW_ORDER,
        ],
    },
    PolicyName.MANUFACTURER_RECORD_MANUFACTURE_EVENT: {
        "description": "Manufacturers can record 'manufactured' events for products they create.",
        "effect": Effect.ALLOW,
        "subject_attributes": [
            (SubjectAttribute.COMPANY_TYPE, "manufacturer"),
        ],
        "resource_attributes": [
            (ResourceAttribute.PRODUCT_OWNER_ID, "${subject.company_id}"),
        ],
        "actions": [
            Action.CREATE_EVENT,
        ],
    },
    PolicyName.DISTRIBUTOR_VIEW_CUSTODY_TRANSFERS: {
        "description": "Distributors can view any custody transfer they are part of.",
        "effect": Effect.ALLOW,
        "subject_attributes": [
            (SubjectAttribute.COMPANY_TYPE, "distributor"),
        ],
        "resource_attributes": [
            (ResourceAttribute.FROM_COMPANY_ID, "${subject.company_id}"),
            (ResourceAttribute.TO_COMPANY_ID, "${subject.company_id}"),
        ],
        "actions": [
            Action.VIEW_CUSTODY_TRANSFER,
        ],
    },
    PolicyName.DISTRIBUTOR_RECORD_CUSTODY_TRANSFER: {
        "description": "Distributors can record custody transfers when handling shipments.",
        "effect": Effect.ALLOW,
        "subject_attributes": [
            (SubjectAttribute.COMPANY_TYPE, "distributor"),
        ],
        "resource_attributes": [
            (ResourceAttribute.FROM_COMPANY_ID, "${subject.company_id}"),
            (ResourceAttribute.TO_COMPANY_ID, "${subject.company_id}"),
        ],
        "actions": [
            Action.CREATE_CUSTODY_TRANSFER,
        ],
    },
    PolicyName.RETAILER_VIEW_ORDERS_THEY_RECEIVE: {
        "description": "Retailers can view orders placed TO them.",
        "effect": Effect.ALLOW,
        "subject_attributes": [
            (SubjectAttribute.COMPANY_TYPE, "retailer"),
        ],
        "resource_attributes": [
            (ResourceAttribute.RECEIVER_COMPANY_ID, "${subject.company_id}"),
        ],
        "actions": [
            Action.VIEW_ORDER,
        ],
    },
    PolicyName.RETAILER_RECORD_SALE_EVENT: {
        "description": "Retailers can record the final sale event for products they own.",
        "effect": Effect.ALLOW,
        "subject_attributes": [
            (SubjectAttribute.COMPANY_TYPE, "retailer"),
        ],
        "resource_attributes": [
            (ResourceAttribute.PRODUCT_OWNER_ID, "${subject.company_id}"),
        ],
        "actions": [
            Action.CREATE_EVENT,
        ],
    },
    PolicyName.RETAILER_RECORD_CUSTODY_TRANSFER: {
        "description": "Retailers record receipt from Distributors.",
        "effect": Effect.ALLOW,
        "subject_attributes": [
            (SubjectAttribute.COMPANY_TYPE, "retailer"),
        ],
        "resource_attributes": [
            (ResourceAttribute.TO_COMPANY_ID, "${subject.company_id}"),
        ],
        "actions": [
            Action.CREATE_CUSTODY_TRANSFER,
        ],
    },
    PolicyName.CONSUMER_VIEW_PRODUCT_PASSPORT: {
        "description": "Consumers can view events only for products they have scanned.",
        "effect": Effect.ALLOW,
        "subject_attributes": [
            (SubjectAttribute.COMPANY_TYPE, "consumer"),
            (SubjectAttribute.SCANNED_PRODUCTS, "${resource.product_key}"),
        ],
        "resource_attributes": [
            (ResourceAttribute.PRODUCT_KEY, "${subject.scanned_products}"),
        ],
        "actions": [
            Action.VIEW_EVENT,
        ],
    },
    PolicyName.ADMIN_FULL_ACCESS: {
        "description": "Superusers can perform any action.",
        "effect": Effect.ALLOW,
        "subject_attributes": [
            (SubjectAttribute.IS_SUPERUSER, "True"),
        ],
        "resource_attributes": [],
        "actions": [
            Action.ALL,
        ],
    },
}
