package dev.qcom.bandmenu

import org.junit.Assert.assertEquals
import org.junit.Test

class BandValidatorTest {

    private val hw = HardwareBands(
        gsm = setOf(850, 900, 1800, 1900),
        wcdma = setOf(1, 3, 8),
        lte = setOf(1, 3, 7, 28),
        nr = setOf(1, 28, 78)
    )

    @Test
    fun validateGsm_keepsOnlyHardwareBands() {
        val result = BandValidator.validateGsm(setOf(850, 900, 700), hw)
        assertEquals(setOf(850, 900), result)
    }

    @Test
    fun validateGsm_emptyHw_returnsEmpty() {
        val result = BandValidator.validateGsm(setOf(850), HardwareBands())
        assertEquals(emptySet<Int>(), result)
    }

    @Test
    fun validateWcdma_keepsOnlyHardwareBands() {
        val result = BandValidator.validateWcdma(setOf(1, 3, 8, 19), hw)
        assertEquals(setOf(1, 3, 8), result)
    }

    @Test
    fun validateLte_keepsOnlyHardwareBands() {
        val result = BandValidator.validateLte(setOf(1, 3, 7, 28, 66), hw)
        assertEquals(setOf(1, 3, 7, 28), result)
    }

    @Test
    fun validateNr_keepsOnlyHardwareBands() {
        val result = BandValidator.validateNr(setOf(1, 28, 78, 257), hw)
        assertEquals(setOf(1, 28, 78), result)
    }

    @Test
    fun validateSimState_filtersAllBands() {
        val state = SimState(
            gsmBands = setOf(850, 700),
            wcdmaBands = setOf(1, 19),
            lteBands = setOf(1, 66),
            nrNsaBands = setOf(1, 257),
            nrSaBands = setOf(28, 257)
        )
        val validated = BandValidator.validateSimState(state, hw)
        assertEquals(setOf(850), validated.gsmBands)
        assertEquals(setOf(1), validated.wcdmaBands)
        assertEquals(setOf(1), validated.lteBands)
        assertEquals(setOf(1), validated.nrNsaBands)
        assertEquals(setOf(28), validated.nrSaBands)
    }
}
