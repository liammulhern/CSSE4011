import { defineStore } from 'pinia'
import http from '@/utils/http'

export interface ProductType {
  id: number
  product_number: string
  name: string
  description: string | null
  created_timestamp: string
  owner: number | null
  recorded_by: number | null
}

interface ProductTypeState {
  list: ProductType[]
  detail: ProductType | null
  loadingList: boolean
  loadingDetail: boolean
  errorList: string | null
  errorDetail: string | null
}

export const useProductTypeStore = defineStore('productType', {
  state: (): ProductTypeState => ({
    list: [],
    detail: null,
    loadingList: false,
    loadingDetail: false,
    errorList: null,
    errorDetail: null,
  }),

  actions: {
    async fetchProductTypes() {
      this.loadingList = true; this.errorList = null;
      try {
        const { data } = await http.get<ProductType[]>('api/producttypes/')
        this.list = data
      } catch (err: any) {
        this.errorList = err.response?.data?.detail || err.message || String(err)
      } finally {
        this.loadingList = false
      }
    },

    async fetchProductType(id: number | string) {
      this.loadingDetail = true; this.errorDetail = null;
      try {
        const { data } = await http.get<ProductType>(`/api/producttypes/${id}/`)
        this.detail = data
      } catch (err: any) {
        this.errorDetail = err.response?.data?.detail || err.message || String(err)
      } finally {
        this.loadingDetail = false
      }
    },

    async createProductType(payload: Omit<ProductType, 'id' | 'created_timestamp' | 'recorded_by'>) {
      try {
        const { data } = await http.post<ProductType>('/api/producttypes/', payload)
        this.list.unshift(data)
        return data
      } catch (err: any) {
        throw new Error(err.response?.data?.detail || err.message || String(err))
      }
    },
  }
})
